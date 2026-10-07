package com.zoavintsoa.secondscreen

import android.content.Context
import android.view.SurfaceHolder
import java.io.BufferedInputStream
import java.io.DataInputStream
import java.net.InetSocketAddress
import java.net.Socket
import java.util.UUID
import java.util.concurrent.Executors
import java.util.concurrent.atomic.AtomicBoolean

class SecondScreenClient(
    private val context:Context,
    private val surface:SurfaceHolder,
    private val onStatus:(String)->Unit,
    private val onPairingRequired:(String)->String?
):SurfaceHolder.Callback {
    private val executor=Executors.newSingleThreadExecutor()
    private val running=AtomicBoolean(false)
    private var socket:Socket?=null
    private var controlSocket:Socket?=null
    private var controlClient:ControlClient?=null
    private var decoder:VideoStreamDecoder?=null
    private var discoveredHost:HostAdvertisement?=null
    private var streamConfig=StreamConfig()
    private val reassembler=VideoFragmentReassembler()
    private val preferences=context.getSharedPreferences("secondscreen",Context.MODE_PRIVATE)
    private val deviceId:String by lazy {
        preferences.getString("deviceId",null) ?: UUID.randomUUID().toString().also{
            preferences.edit().putString("deviceId",it).apply()
        }
    }

    override fun surfaceCreated(holder:SurfaceHolder){start()}
    override fun surfaceChanged(holder:SurfaceHolder,format:Int,width:Int,height:Int)=Unit
    override fun surfaceDestroyed(holder:SurfaceHolder){stop()}

    private fun start(){
        if(!running.compareAndSet(false,true))return
        executor.execute{connectLoop()}
    }

    private fun openControl(host:HostAdvertisement):ControlClient {
        val cs=Socket().apply{
            tcpNoDelay=true
            connect(InetSocketAddress(host.address,host.controlPort),1500)
        }
        controlSocket=cs
        return ControlClient(cs).also{controlClient=it}
    }

    private fun connectLoop(){
        while(running.get()){
            try{
                onStatus("SecondScreen — recherche d’un hôte…")
                val host=discoverHost() ?: error("No SecondScreen host found")
                val capabilities=DeviceCapabilitiesProbe.probe(context)

                var cc=openControl(host)
                if(!cc.hello(deviceId,"Android SecondScreen",DeviceCapabilitiesProbe.helloJson(capabilities))){
                    error("Control HELLO rejected")
                }

                var authenticated=false
                val stored=preferences.getString("sessionToken",null)
                if(stored!=null){
                    val result=runCatching{cc.authenticate(stored)}.getOrNull()
                    if(result?.authenticated==true){
                        authenticated=true
                        result.streamConfig?.let{streamConfig=it.normalized()}
                    } else {
                        // An invalid/expired token causes the host session to close.
                        // Never reuse that TCP socket for a new pairing attempt.
                        controlClient?.close()
                        controlClient=null
                        controlSocket=null
                        cc=openControl(host)
                        if(!cc.hello(deviceId,"Android SecondScreen",DeviceCapabilitiesProbe.helloJson(capabilities))){
                            error("Control HELLO rejected after token reset")
                        }
                    }
                }

                if(!authenticated){
                    onStatus("SecondScreen — appairage requis")
                    val code=onPairingRequired(host.name) ?: error("Pairing cancelled")
                    val result=cc.pair(code.trim())
                    if(!result.authenticated || result.sessionToken==null) error("Pairing rejected")
                    preferences.edit().putString("sessionToken",result.sessionToken).apply()
                    result.streamConfig?.let{streamConfig=it.normalized()}
                    authenticated=true
                }

                onStatus("SecondScreen — sécurisé, connexion vidéo…")
                val s=Socket().apply{
                    tcpNoDelay=true
                    connect(InetSocketAddress(host.address,host.videoPort),1500)
                }
                socket=s
                consumeVideo(DataInputStream(BufferedInputStream(s.getInputStream())))
            }catch(t:Throwable){
                onStatus("SecondScreen — reconnexion…")
                if(running.get())Thread.sleep(1000)
            }finally{
                socket?.runCatching{close()};socket=null
                controlClient?.runCatching{close()};controlClient=null
                controlSocket?.runCatching{close()};controlSocket=null
                decoder?.release();decoder=null
                reassembler.reset()
            }
        }
    }

    fun acceptQuicDatagram(datagram:ByteArray){
        if(!running.get())return
        val frame=reassembler.accept(datagram)
        if(frame!=null){
            streamConfig=streamConfig.copy(codec=frame.codec).normalized()
            decode(frame.codec,streamConfig.width,streamConfig.height,frame.annexB,frame.timestampUs,(frame.flags and 1)!=0)
        }
    }

    private fun discoverHost():HostAdvertisement?{
        discoveredHost=DiscoveryClient().discover()?:discoveredHost
        return discoveredHost
    }

    private fun consumeVideo(input:DataInputStream){
        while(running.get()){
            val magic=ByteArray(4);input.readFully(magic)
            when(String(magic,Charsets.US_ASCII)){
                "SSVF"->{
                    val codec=input.readUnsignedByte()
                    val flags=input.readUnsignedByte()
                    input.readUnsignedShort()
                    val timestamp=input.readLong()
                    val length=input.readInt()
                    require(length in 1..16_777_216)
                    val au=ByteArray(length);input.readFully(au)
                    decode(codec,streamConfig.width,streamConfig.height,au,timestamp,(flags and 1)!=0)
                }
                "SSVG"->{
                    controlClient?.requestKeyframe()
                    error("SSVG requires QUIC datagrams")
                }
                else->error("Unknown video frame magic")
            }
        }
    }

    private fun decode(codec:Int,width:Int,height:Int,au:ByteArray,timestampUs:Long,keyFrame:Boolean){
        if(decoder==null)decoder=VideoStreamDecoder(surface)
        runCatching{
            decoder?.decode(codec,width,height,au,timestampUs,keyFrame)
        }.onFailure{
            controlClient?.requestKeyframe()
        }
    }

    fun close(){stop();executor.shutdownNow()}

    private fun stop(){
        running.set(false)
        socket?.runCatching{close()}
        controlSocket?.runCatching{close()}
        decoder?.release();decoder=null
    }
}
