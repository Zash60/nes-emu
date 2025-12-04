package com.example.nesemu
import android.graphics.Bitmap
class NesEmulator {
    companion object { init { System.loadLibrary("nes-emu") } }
    external fun init()
    external fun loadRom(path: String)
    external fun runFrame(bitmap: Bitmap, rewind: Boolean)
    external fun setButtonState(buttonId: Int, pressed: Boolean)
    external fun saveState(save: Boolean)
}
