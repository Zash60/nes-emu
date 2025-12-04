package com.example.nesemu

import android.graphics.Bitmap

class NesEmulator {
    companion object {
        init {
            System.loadLibrary("nes-emu")
        }
    }

    external fun init()
    // AGORA RECEBE BYTE ARRAY
    external fun loadRom(romData: ByteArray)
    external fun runFrame(bitmap: Bitmap, rewind: Boolean)
    external fun setButtonState(buttonId: Int, pressed: Boolean)
    external fun saveState(save: Boolean)
}
