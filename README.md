# Implementación y Optimización de un Modelo YOLO en la Plataforma Embebida LicheeRV Nano (SG2002)

Repositorio oficial del trabajo de grado titulado: *"Implementación y optimización de un modelo YOLO en la plataforma embebida LicheeRV Nano (SG2002) para detección de objetos con documentación replicable en entornos educativos"*, presentado por **Jesús Antonio López Trigozo** ante la Facultad de Ingeniería de la **Universidad Nacional de Colombia (Sede Bogotá)**, bajo la dirección del profesor Carlos Iván Camargo Bareño.

> **Estado del Proyecto:** 🟢 En desarrollo (Avance actual estimado: **60%** - Entorno configurado, toolchain de NPU integrado y scripts base de inferencia implementados).

---

## 📌 1. Introducción y Justificación
El despliegue de Inteligencia Artificial en el borde (*Edge AI*) en hardware de bajo costo suele verse obstaculizado por la escasez de guías técnicas claras y reproducibles. Este proyecto aborda dicha problemática implementando un modelo YOLO optimizado sobre la placa **LicheeRV Nano (SoC SG2002)**, aprovechando su NPU nativa de 1 TOPS INT8, su interfaz de cámara CSI y conectividad Wi-Fi. El propósito central es entregar a la comunidad académica una metodología abierta, paso a paso, para facilitar el desarrollo de sistemas embebidos inteligentes.

---

## 🛠️ 2. Especificaciones Técnicas del Sistema
* **Plataforma:** LicheeRV Nano (SoC Sophgo SG2002: Dual RISC-V C906 a 1 GHz / 700 MHz, 256MB RAM DDR3).
* **Aceleración de IA:** NPU integrada con rendimiento de 1 TOPS (precisión INT8/BF16).
* **Periféricos utilizados:** Módulo de cámara CSI, antena Wi-Fi de doble banda (N2425D) y tarjeta MicroSD (Clase 10, formato MBR).
* **Software Base:** Buildroot Linux oficial (Soporte nativo para drivers de cámara y NPU).

---

## 🚀 3. Guía de Réplica (Paso a Paso)

### Fase 1: Preparación del Entorno y Sistema Operativo (Buildroot)
1. **Descarga y flasheo de la imagen Buildroot** en la tarjeta MicroSD utilizando un sistema Linux:
   ```bash
   xzcat buildroot_licheervnano.img.xz | sudo dd of=/dev/sdX conv=sync status=progress
   ```
   *(Asegúrate de reemplazar `/dev/sdX` con el identificador correcto de tu tarjeta MicroSD)*.

2. **Configuración de Red Wi-Fi previa al primer arranque:**
   Monta la partición `rootfs` de la tarjeta SD y edita el archivo `/etc/wpa_supplicant.conf` para configurar las credenciales de tu red:
   ```text
   ctrl_interface=/var/run/wpa_supplicant
   ap_scan=1
   network={
       ssid="TU_RED_WIFI"
       psk="TU_CONTRASEÑA"
   }
   ```

3. **Conexión e Inicio de Sesión:**
   Inserta la tarjeta MicroSD en la placa, enciéndela y localiza su IP en la red local mediante `nmap`:
   ```bash
   sudo nmap -sV -p 22 192.168.1.0/24
   ```
   Conéctate a la placa vía SSH (Credenciales por defecto: usuario `root`):
   ```bash
   ssh root@<IP_DE_LA_PLACA>
   ```

---

### Fase 2: Cuantización y Compilación del Modelo YOLO para la NPU
Para lograr inferencia en tiempo real en el chip SG2002, es necesario compilar los pesos del modelo utilizando el kit de desarrollo (SDK) de SOPHGO:
1. Exporta tu arquitectura YOLO (ej. YOLOv8n) desde PyTorch a formato **ONNX**.
2. Emplea las herramientas de cuantización a **INT8** provistas en el SDK del fabricante para generar el archivo optimizado compatible con la NPU (`.cvimodel`).
3. Transfiere el modelo generado a la placa LicheeRV Nano mediante SCP:
   ```bash
   scp modelo_int8.cvimodel root@<IP_DE_LA_PLACA>:/root/
   ```

---

### Fase 3: Pipeline de Visión por Computadora (En Desarrollo / 60%)
* [x] Configuración de drivers del kernel para la cámara CSI.
* [x] Integración inicial de las librerías del Multimedia Framework (MMF) y CVI TDL SDK.
* [ ] Pruebas finales de estabilidad de fotogramas por segundo (FPS) bajo carga continua.
* [ ] Automatización de scripts de ejecución al arranque del sistema.

Ejemplo básico de inicialización del pipeline de captura e inferencia local en la placa:
```python
# Script base preliminar para la captura e inferencia (En fase de optimización)
import cv2
import sys

def main():
    print("Iniciando pipeline de video en LicheeRV Nano...")
    cap = cv2.VideoCapture(0)
    
    while cap.isOpened():
        ret, frame = cap.read()
        if not ret:
            break
        # [Espacio reservado para la ejecución de inferencia con el NPU]
        
        cv2.imshow("Edge AI - UNAL", frame)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
            
    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
```

---

## 📊 4. Estado de Avance y Tareas Pendientes (Cronograma)
* **Semanas 1 - 4:** Configuración de hardware, flasheo de Buildroot y conectividad Wi-Fi/SSH — **(Completado 100%)**
* **Semanas 5 - 8:** Cuantización de modelos YOLO a INT8 y pruebas de compilación cruzada con el SDK — **(Completado 100%)**
* **Semanas 9 - 12:** Desarrollo del pipeline de cámara, integración con la NPU y pruebas de rendimiento — **(En progreso - ~50%)**
* **Semanas 13 - 16:** Consolidación de la documentación final en GitHub y sustentación — **(Pendiente)**

---

## 📜 5. Licencia y Créditos
Proyecto desarrollado bajo los parámetros institucionales de la **Universidad Nacional de Colombia**. 
* **Autor:** Jesús Antonio López Trigozo
* **Director:** Carlos Iván Camargo Bareño
