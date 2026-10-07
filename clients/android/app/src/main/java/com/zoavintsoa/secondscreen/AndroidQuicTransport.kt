package com.zoavintsoa.secondscreen

interface AndroidQuicTransport {
    val available:Boolean
    fun connect(host:String,port:Int,alpn:String="secondscreen/1"):Boolean
    fun sendControl(frame:ByteArray):Boolean
    fun sendKeyframeRequest():Boolean
    fun close()
    fun setDatagramListener(listener:(ByteArray)->Unit)
}

class NativeMsQuicTransport:AndroidQuicTransport {
    private var listener:((ByteArray)->Unit)?=null
    override val available:Boolean get()=nativeAvailable()
    override fun connect(host:String,port:Int,alpn:String)=available && nativeConnect(host,port,alpn)
    override fun sendControl(frame:ByteArray)=available && nativeSendControl(frame)
    override fun sendKeyframeRequest()=available && nativeSendKeyframeRequest()
    override fun close(){if(available) nativeClose()}
    override fun setDatagramListener(listener:(ByteArray)->Unit){this.listener=listener}
    fun deliverDatagram(data:ByteArray){listener?.invoke(data)}
    private external fun nativeAvailable():Boolean
    private external fun nativeConnect(host:String,port:Int,alpn:String):Boolean
    private external fun nativeSendControl(frame:ByteArray):Boolean
    private external fun nativeSendKeyframeRequest():Boolean
    private external fun nativeClose()
    companion object { init { runCatching{System.loadLibrary("secondscreen_quic")} } }
}
