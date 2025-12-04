package com.example.nesemu
import android.annotation.SuppressLint
import android.graphics.Bitmap
import android.os.Bundle
import android.view.MotionEvent
import android.view.View
import android.widget.Button
import android.widget.ImageView
import androidx.appcompat.app.AppCompatActivity
import kotlinx.coroutines.*
import java.io.File
class MainActivity : AppCompatActivity() {
    private val emu = NesEmulator()
    private lateinit var bmp: Bitmap
    private var run = false
    private var rew = false
    @SuppressLint("ClickableViewAccessibility")
    override fun onCreate(b: Bundle?) {
        super.onCreate(b)
        setContentView(R.layout.activity_main)
        val img = findViewById<ImageView>(R.id.sv)
        bmp = Bitmap.createBitmap(256, 240, Bitmap.Config.ARGB_8888)
        img.setImageBitmap(bmp)
        emu.init()
        // Copia dummy rom se necessario e carrega
        val f = File(cacheDir, "game.nes")
        if(f.exists()) emu.loadRom(f.absolutePath)
        
        setupBtns()
        run = true
        CoroutineScope(Dispatchers.Default).launch {
            while(run) {
                val t = System.currentTimeMillis()
                emu.runFrame(bmp, rew)
                withContext(Dispatchers.Main){ img.invalidate() }
                val w = 16 - (System.currentTimeMillis()-t)
                if(w>0) delay(w)
            }
        }
    }
    @SuppressLint("ClickableViewAccessibility")
    fun setupBtns() {
        val ids = listOf(R.id.ba, R.id.bb, R.id.bsel, R.id.bstr, R.id.bu, R.id.bd, R.id.bl, R.id.br)
        for((i, id) in ids.withIndex()) {
            findViewById<View>(id).setOnTouchListener { _, e ->
                if(e.action==MotionEvent.ACTION_DOWN) emu.setButtonState(i, true)
                else if(e.action==MotionEvent.ACTION_UP) emu.setButtonState(i, false)
                true
            }
        }
        findViewById<Button>(R.id.brew).setOnTouchListener { _, e -> rew = (e.action==MotionEvent.ACTION_DOWN); true }
        findViewById<Button>(R.id.bsave).setOnClickListener { emu.saveState(true) }
        findViewById<Button>(R.id.bload).setOnClickListener { emu.saveState(false) }
    }
}
