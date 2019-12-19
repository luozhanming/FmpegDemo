package cn.luozhanming.opengldemo

import android.os.Bundle
import android.os.Environment
import android.widget.Button
import android.widget.EditText
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import java.util.jar.Manifest

class MainActivity : AppCompatActivity() {

    val input by lazy {
        findViewById<EditText>(R.id.input)
    }

    val output by lazy {
        findViewById<EditText>(R.id.output)
    }

    val btnDecode by lazy {
        findViewById<Button>(R.id.btn_decode)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        ActivityCompat.requestPermissions(this, arrayOf(android.Manifest.permission.WRITE_EXTERNAL_STORAGE,android.Manifest.permission.READ_EXTERNAL_STORAGE),1)
        btnDecode.setOnClickListener {
            val test = FFmpgeTest()
            val inputStr = "${Environment.getExternalStorageDirectory().absoluteFile}/${input.text.toString()}"
            val outputStr = "${Environment.getExternalStorageDirectory().absoluteFile}/${output.text.toString()}"
            object :Thread(){
                override fun run() {
                    test.decode(inputStr,outputStr)
                }
            }.start()
        }

    }
}
