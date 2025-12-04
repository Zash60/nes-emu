package com.example.nesemu

import android.annotation.SuppressLint
import android.graphics.Bitmap
import android.net.Uri
import android.os.Bundle
import android.view.MotionEvent
import android.view.View
import android.widget.Button
import android.widget.ImageView
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import kotlinx.coroutines.*

class MainActivity : AppCompatActivity() {

    private val emulator = NesEmulator()
    private lateinit var screenView: ImageView
    private lateinit var bitmap: Bitmap
    private var isRunning = false
    private var isRewinding = false
    private var isRomLoaded = false

    private val selectRomLauncher = registerForActivityResult(ActivityResultContracts.OpenDocument()) { uri: Uri? ->
        uri?.let { loadRomFromUri(it) }
    }

    @SuppressLint("ClickableViewAccessibility")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        screenView = findViewById(R.id.sv)
        bitmap = Bitmap.createBitmap(256, 240, Bitmap.Config.ARGB_8888)
        screenView.setImageBitmap(bitmap)

        emulator.init()
        setupButtons()

        findViewById<Button>(R.id.btnSelectRom).setOnClickListener {
            selectRomLauncher.launch(arrayOf("*/*"))
        }

        isRunning = true
        CoroutineScope(Dispatchers.Default).launch {
            while (isRunning) {
                if (isRomLoaded) {
                    val startTime = System.currentTimeMillis()
                    emulator.runFrame(bitmap, isRewinding)
                    withContext(Dispatchers.Main) { screenView.invalidate() }
                    val elapsed = System.currentTimeMillis() - startTime
                    if (16 - elapsed > 0) delay(16 - elapsed)
                } else {
                    delay(100)
                }
            }
        }
    }

    private fun loadRomFromUri(uri: Uri) {
        try {
            // LER BYTES DIRETAMENTE (Funciona em todas as versões do Android)
            val inputStream = contentResolver.openInputStream(uri)
            val bytes = inputStream?.readBytes()
            
            if (bytes != null && bytes.isNotEmpty()) {
                emulator.loadRom(bytes)
                isRomLoaded = true
                Toast.makeText(this, "ROM Carregada! (${bytes.size} bytes)", Toast.LENGTH_SHORT).show()
            } else {
                Toast.makeText(this, "Erro: Arquivo vazio", Toast.LENGTH_SHORT).show()
            }
            inputStream?.close()

        } catch (e: Exception) {
            e.printStackTrace()
            Toast.makeText(this, "Erro ao ler arquivo: ${e.message}", Toast.LENGTH_LONG).show()
        }
    }

    @SuppressLint("ClickableViewAccessibility")
    private fun setupButtons() {
        val ids = listOf(R.id.ba, R.id.bb, R.id.bsel, R.id.bstr, R.id.bu, R.id.bd, R.id.bl, R.id.br)
        for ((i, id) in ids.withIndex()) {
            findViewById<View>(id).setOnTouchListener { _, event ->
                if (event.action == MotionEvent.ACTION_DOWN) emulator.setButtonState(i, true)
                else if (event.action == MotionEvent.ACTION_UP) emulator.setButtonState(i, false)
                true
            }
        }
        findViewById<Button>(R.id.brew).setOnTouchListener { _, e -> isRewinding = (e.action == MotionEvent.ACTION_DOWN); true }
        findViewById<Button>(R.id.bsave).setOnClickListener { emulator.saveState(true) }
        findViewById<Button>(R.id.bload).setOnClickListener { emulator.saveState(false) }
    }
}
