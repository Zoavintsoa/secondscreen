package com.secondscreen.client.ui

import android.content.Context
import android.util.AttributeSet
import android.view.MotionEvent
import android.view.SurfaceHolder
import android.view.SurfaceView

class SurfaceRenderView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : SurfaceView(context, attrs, defStyleAttr), SurfaceHolder.Callback {

    var onSurfaceReadyListener: ((SurfaceHolder) -> Unit)? = null
    var onSurfaceDestroyedListener: (() -> Unit)? = null
    var onInputEventListener: ((actionType: Byte, normX: Float, normY: Float, pressure: Float, button: Byte) -> Unit)? = null

    init {
        holder.addCallback(this)
        setZOrderMediaOverlay(true)
        isFocusable = true
        isFocusableInTouchMode = true
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        onSurfaceReadyListener?.invoke(holder)
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {}

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        onSurfaceDestroyedListener?.invoke()
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        val w = width.toFloat()
        val h = height.toFloat()
        if (w <= 0 || h <= 0) return super.onTouchEvent(event)

        val normX = (event.x / w).coerceIn(0f, 1f)
        val normY = (event.y / h).coerceIn(0f, 1f)
        val pressure = event.pressure.coerceIn(0f, 1f)

        val actionType: Byte = when (event.actionMasked) {
            MotionEvent.ACTION_DOWN, MotionEvent.ACTION_POINTER_DOWN -> 0.toByte()
            MotionEvent.ACTION_MOVE -> 1.toByte()
            MotionEvent.ACTION_UP, MotionEvent.ACTION_POINTER_UP, MotionEvent.ACTION_CANCEL -> 2.toByte()
            else -> return super.onTouchEvent(event)
        }

        val button: Byte = if (event.buttonState and MotionEvent.BUTTON_SECONDARY != 0) 2.toByte() else 0.toByte()

        onInputEventListener?.invoke(actionType, normX, normY, pressure, button)
        return true
    }
}
