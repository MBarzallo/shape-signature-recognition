package com.example.appshapesignature

import android.graphics.Bitmap
import android.os.Bundle
import android.widget.Button
import android.widget.LinearLayout
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import java.io.File
import java.io.FileOutputStream
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale


class MainActivity : AppCompatActivity() {

    companion object {
        init { System.loadLibrary("native-lib") }
    }

    var lastPrediction = ""

    external fun classifyBitmapRGBA(
        w: Int,
        h: Int,
        pixels: IntArray,
        assetManager: android.content.res.AssetManager
    ): String

    external fun updateConfusionMatrix(
        realLabel: String,
        predictedLabel: String
    )
    external fun getMetrics(): String


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val drawView = DrawView(this)
        val resultTv = TextView(this).apply {
            textSize = 18f
            setPadding(24, 16, 24, 16)
            text = "Dibuja una figura"
        }
        val btnClassify = styledButton("CLASIFICAR") {
            val bmp = drawView.exportBitmap().copy(Bitmap.Config.ARGB_8888, false)
            val pixels = IntArray(bmp.width * bmp.height)
            bmp.getPixels(pixels, 0, bmp.width, 0, 0, bmp.width, bmp.height)

            val pred = classifyBitmapRGBA(
                bmp.width,
                bmp.height,
                pixels,
                assets
            )

            lastPrediction = pred
            saveBitmap(bmp, pred)
            resultTv.text = "Predicción: $pred"
        }

        val realButtonsLayout = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            weightSum = 3f
        }
        realButtonsLayout.addView(
            styledButton("● Círculo") {
                updateConfusionMatrix("circulo", lastPrediction)
                resultTv.text = "Registrado: círculo"
            },
            LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f)
        )

        realButtonsLayout.addView(
            styledButton("■ Cuadrado") {
                updateConfusionMatrix("cuadrado", lastPrediction)
                resultTv.text = "Registrado: cuadrado"
            },
            LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f)
        )

        realButtonsLayout.addView(
            styledButton("▲ Triángulo") {
                updateConfusionMatrix("triangulo", lastPrediction)
                resultTv.text = "Registrado: triángulo"
            },
            LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f)
        )

        val btnMetrics = styledButton("VER MÉTRICAS") {
            resultTv.text = getMetrics()
        }

        val btnClear = styledButton("LIMPIAR") {
            drawView.clear()
            resultTv.text = "Dibuja una figura"
        }
        val layout = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL

            addView(drawView, LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, 0, 1f
            ))

            addView(resultTv)
            addView(btnClassify)
            addView(realButtonsLayout)
            addView(btnMetrics)
            addView(btnClear)
        }


        setContentView(layout)
    }

    private fun saveBitmap(bitmap: Bitmap, label: String) {
        val dir = File(getExternalFilesDir(null), "shapes")
        if (!dir.exists()) dir.mkdirs()

        val time = SimpleDateFormat("yyyyMMdd_HHmmss", Locale.US).format(Date())
        val file = File(dir, "${label}_$time.png")

        FileOutputStream(file).use { out ->
            bitmap.compress(Bitmap.CompressFormat.PNG, 100, out)
        }
    }

    fun styledButton(text: String, onClick: () -> Unit): Button {
        return Button(this).apply {
            this.text = text
            textSize = 16f
            setPadding(20, 16, 20, 16)
            setOnClickListener { onClick() }
        }
    }


}

