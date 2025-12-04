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
import java.io.File
import java.io.FileOutputStream

class MainActivity : AppCompatActivity() {

    private val emulator = NesEmulator()
    private lateinit var screenView: ImageView
    private lateinit var bitmap: Bitmap
    private var isRunning = false
    private var isRewinding = false
    private var isRomLoaded = false

    // Seletor de Arquivos (Storage Access Framework)
    private val selectRomLauncher = registerForActivityResult(ActivityResultContracts.OpenDocument()) { uri: Uri? ->
        uri?.let {
            loadRomFromUri(it)
        }
    }

    @SuppressLint("ClickableViewAccessibility")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        screenView = findViewById(R.id.sv)
        // NES Resolução 256x240
        bitmap = Bitmap.createBitmap(256, 240, Bitmap.Config.ARGB_8888)
        screenView.setImageBitmap(bitmap)

        emulator.init()
        
        setupButtons()

        // Loop principal do emulador
        isRunning = true
        CoroutineScope(Dispatchers.Default).launch {
            while (isRunning) {
                if (isRomLoaded) {
                    val startTime = System.currentTimeMillis()

                    emulator.runFrame(bitmap, isRewinding)

                    withContext(Dispatchers.Main) {
                        screenView.invalidate() // Força redesenho
                    }

                    // Limiter simples ~60 FPS
                    val elapsed = System.currentTimeMillis() - startTime
                    val wait = 16 - elapsed
                    if (wait > 0) delay(wait)
                } else {
                    // Espera carregar a ROM
                    delay(100)
                }
            }
        }
    }

    private fun loadRomFromUri(uri: Uri) {
        try {
            // O código C++ usa fopen(), que não entende content:// URIs do Android.
            // Precisamos copiar o arquivo para o cache privado do aplicativo.
            val inputStream = contentResolver.openInputStream(uri)
            val fileName = "temp_game.nes"
            val cacheFile = File(cacheDir, fileName)

            inputStream?.use { input ->
                FileOutputStream(cacheFile).use { output ->
                    input.copyTo(output)
                }
            }

            // Agora carregamos do caminho do arquivo local
            emulator.loadRom(cacheFile.absolutePath)
            isRomLoaded = true
            Toast.makeText(this, "ROM Carregada!", Toast.LENGTH_SHORT).show()

        } catch (e: Exception) {
            e.printStackTrace()
            Toast.makeText(this, "Erro ao carregar ROM", Toast.LENGTH_SHORT).show()
        }
    }

    @SuppressLint("ClickableViewAccessibility")
    private fun setupButtons() {
        // Botão para selecionar ROM
        findViewById<Button>(R.id.btnSelectRom).setOnClickListener {
            selectRomLauncher.launch(arrayOf("*/*")) // Filtro genérico, pode usar application/octet-stream
        }

        val ids = listOf(R.id.ba, R.id.bb, R.id.bsel, R.id.bstr, R.id.bu, R.id.bd, R.id.bl, R.id.br)
        // Mapeamento: 0=A, 1=B, 2=Select, 3=Start, 4=Up, 5=Down, 6=Left, 7=Right
        for ((i, id) in ids.withIndex()) {
            findViewById<View>(id).setOnTouchListener { _, event ->
                when (event.action) {
                    MotionEvent.ACTION_DOWN -> emulator.setButtonState(i, true)
                    MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> emulator.setButtonState(i, false)
                }
                true
            }
        }

        findViewById<Button>(R.id.bsave).setOnClickListener { emulator.saveState(true) }
        findViewById<Button>(R.id.bload).setOnClickListener { emulator.saveState(false) }

        val btnRewind = findViewById<Button>(R.id.brew)
        btnRewind.setOnTouchListener { _, event ->
            when (event.action) {
                MotionEvent.ACTION_DOWN -> isRewinding = true
                MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> isRewinding = false
            }
            true
        }
    }
}
