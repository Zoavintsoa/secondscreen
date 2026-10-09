package com.zoavintsoa.secondscreen

data class StreamConfig(
    val width:Int=1280,val height:Int=720,val fps:Int=30,val bitrateKbps:Int=4000,
    val profile:String="Safe",val codec:Int=1
){
    fun normalized()=copy(width=width.coerceIn(320,3840),height=height.coerceIn(240,2160),
        fps=fps.coerceIn(15,120),bitrateKbps=bitrateKbps.coerceIn(500,50000),codec=if(codec==2)2 else 1)
    companion object {
        fun parse(json:String):StreamConfig {
            fun i(n:String,d:Int)=Regex("\"$n\"\\s*:\\s*(\\d+)").find(json)?.groupValues?.get(1)?.toIntOrNull()?:d
            fun s(n:String,d:String)=Regex("\"$n\"\\s*:\\s*\"([^\"]+)\"").find(json)?.groupValues?.get(1)?:d
            val codecText=s("codec","").lowercase()
            val codec=if(codecText=="hevc" || codecText=="2")2 else if(codecText=="h264" || codecText=="avc" || codecText=="1")1 else i("codec",1)
            return StreamConfig(
                i("width",1280),i("height",720),i("fps",30),i("bitrateKbps",4000),
                s("profile",s("profileId","Safe")),codec
            ).normalized()
        }
    }
}
