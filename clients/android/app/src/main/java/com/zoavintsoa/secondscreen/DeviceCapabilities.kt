package com.zoavintsoa.secondscreen

import android.content.Context
import android.media.MediaCodecList
import android.os.Build
import android.view.WindowManager
import kotlin.math.min

data class VideoCapability(val codec:Int,val mime:String,val maxWidth:Int,val maxHeight:Int,val maxFps:Double)
data class DeviceCapabilities(
    val api:Int,val model:String,val manufacturer:String,val touch:Boolean,val stylus:Boolean,
    val refreshHz:Float,val hdr:Boolean,val codecs:List<VideoCapability>
) { fun bestBaseline()=codecs.firstOrNull{it.codec==1} ?: codecs.firstOrNull() }

object DeviceCapabilitiesProbe {
    fun helloJson(c:DeviceCapabilities):String {
        val codecs=c.codecs.joinToString(","){ x -> "{\"codec\":"+x.codec+",\"maxWidth\":"+x.maxWidth+",\"maxHeight\":"+x.maxHeight+",\"maxFps\":"+x.maxFps+"}" }
        return "{\"platform\":\"android\",\"api\":"+c.api+",\"model\":\""+escape(c.model)+"\",\"manufacturer\":\""+escape(c.manufacturer)+"\",\"touch\":"+c.touch+",\"stylus\":"+c.stylus+",\"refreshHz\":"+c.refreshHz+",\"hdr\":"+c.hdr+",\"codecs\":["+codecs+"]}"
    }

    fun probe(context:Context) {
        val pm=context.packageManager
        val display=if(Build.VERSION.SDK_INT>=30) context.getSystemService(WindowManager::class.java)?.defaultDisplay
        else @Suppress("DEPRECATION") (context.getSystemService(Context.WINDOW_SERVICE) as? WindowManager)?.defaultDisplay
        val refresh=display?.refreshRate ?: 60f
        val hdr=if(Build.VERSION.SDK_INT>=24) display?.hdrCapabilities?.supportedHdrTypes?.isNotEmpty()==true else false
        val touch=pm.hasSystemFeature("android.hardware.touchscreen")
        val stylus=if(Build.VERSION.SDK_INT>=23) pm.hasSystemFeature("android.hardware.touchscreen.stylus") else false
        val codecs=mutableListOf<VideoCapability>()
        for(info in MediaCodecList(MediaCodecList.ALL_CODECS).codecInfos) {
            if(info.isEncoder) continue
            for(type in info.supportedTypes) {
                val mime=type.lowercase()
                val codec=when(mime){"video/avc"->1;"video/hevc"->2;else->continue}
                val caps=runCatching{info.getCapabilitiesForType(type)}.getOrNull() ?: continue
                val vc=caps.videoCapabilities ?: continue
                val width=vc.supportedWidths.upper
                val height=vc.supportedHeights.upper
                val fps=runCatching{vc.getSupportedFrameRatesFor(width,height).upper}.getOrDefault(60.0)
                codecs += VideoCapability(codec,mime,width,height,fps)
            }
        }
        return DeviceCapabilities(Build.VERSION.SDK_INT,Build.MODEL?:"Android",Build.MANUFACTURER?:"unknown",
            touch,stylus,min(refresh,240f),hdr,codecs.distinctBy{Triple(it.codec,it.maxWidth,it.maxHeight)})
    }

    private fun escape(v:String)=v.replace("\\","\\\\").replace("\"","\\\"")
}
