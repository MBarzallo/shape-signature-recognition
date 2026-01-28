# Shape Signature Recognition (Android + OpenCV)

Repositorio del proyecto de Visión por Computador enfocado en el reconocimiento de figuras geométricas
(círculo, cuadrado y triángulo) mediante **firma de contornos** y **Transformada Discreta de Fourier (DFT)**,
implementado en **Android (Kotlin)** con procesamiento nativo en **C++ (OpenCV + JNI)**.

---

## Integrantes
- Mateo Barzallo
- Karen Quito

## Descripción general
El sistema permite al usuario dibujar una figura en pantalla. A partir de este dibujo se:
1. Extrae el contorno principal.
2. Calcula la firma del contorno centrada en el centroide.
3. Aplica la DFT para obtener un descriptor invariante.
4. Clasifica la figura comparando con un dataset de referencia.
5. Evalúa el desempeño mediante matriz de confusión y accuracy, usando validación manual.

---

## Estructura del proyecto

```
.
├── app/
│   ├── src/main/java/           # Código Kotlin (UI, DrawView, MainActivity)
│   ├── src/main/cpp/            # Código nativo C++ (OpenCV, JNI)
│   ├── src/main/assets/dataset/ # Dataset de entrenamiento (imágenes)
│   └── src/main/res/            # Recursos Android
├── VisionP3.ipynb               # Cuaderno Jupyter (Parte 1 - análisis teórico/experimental)
├── Informe/                     # Informe en LaTeX (PDF final)
└── README.md
```

---

## Dataset
El dataset está ubicado en:
```
app/src/main/assets/dataset/
```

Formato de nombres de archivo:
```
c#-$.jpg  -> círculo
q#-$.jpg  -> cuadrado
t#-$.jpg  -> triángulo
```

---

## Evaluación
La evaluación se realiza mediante:
- Matriz de confusión
- Accuracy global

El usuario indica manualmente la clase real desde la interfaz de la aplicación, lo que permite
actualizar las métricas sin reiniciar la aplicación.

---

## Tecnologías utilizadas
- Kotlin (Android)
- C++ (JNI)
- OpenCV
- Android NDK / CMake
- Jupyter Notebook (Parte teórica)
- LaTeX (Informe)

---
