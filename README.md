# Implementación y Optimización de un Modelo YOLO en la Plataforma Embebida LicheeRV Nano (SG2002)

Repositorio oficial del trabajo de grado titulado: *"Implementación y optimización de un modelo YOLO en la plataforma embebida LicheeRV Nano (SG2002) para detección de objetos con documentación replicable en entornos educativos"*, presentado por **Jesús Antonio López Trigozo** ante la Facultad de Ingeniería de la **Universidad Nacional de Colombia (Sede Bogotá)**, bajo la dirección del profesor Carlos Iván Camargo Bareño.

> **Estado del Proyecto:** 🟢 En desarrollo (Avance actual estimado: **60%** - Entorno configurado, toolchain de NPU integrado y scripts base de inferencia implementados).

---

## 📌 1. Introducción y Justificación
El despliegue de Inteligencia Artificial en el borde (*Edge AI*) en hardware de bajo costo suele verse obstaculizado por la escasez de guías técnicas claras y reproducibles. Este proyecto aborda dicha problemática implementando un modelo YOLO optimizado sobre la placa **LicheeRV Nano (SoC SG2002)**, aprovechando su NPU nativa de 1 TOPS INT8, su interfaz de cámara CSI y conectividad Wi-Fi. El propósito central es entregar a la comunidad académica una metodología abierta, paso a paso, para facilitar el desarrollo de sistemas embebidos inteligentes.

---

## 🛠️ 2. Especificaciones Técnicas y Versiones

### Tabla de Especificaciones Generales
| Elemento | Especificación |
| :--- | :--- |
| **CPU** | SOPHGO SG2002 <br>• Núcleo principal: 1GHz RISC-V C906 / ARM A53 (seleccionable) <br>• Núcleo secundario: 700MHz RISC-V C906 <br>• Núcleo de bajo consumo: 25–300M 8051 |
| **Memoria** | DDR3 integrada de 2Gbit (256MByte) |
| **Almacenamiento** | Arranque desde tarjeta TF / SD NAND (pads SD NAND bajo el slot de tarjeta TF) |
| **Aceleración de IA** | NPU integrada de **1 TOPS INT8** (soporta BF16) |
| **Interfaz de Video** | • Salida: MIPI DSI de 2 líneas (interfaz estándar de 31 pines, soporte táctil capacitivo de 6 pines) <br>• Entrada: MIPI CSI de 4 líneas (interfaz de 22 pines, soporte de división CSI dual) |
| **Interfaz de Audio** | • Salida: Amplificador PA incorporado (conexión directa a altavoces <1W) <br>• Entrada: Micrófono de silicio analógico integrado |
| **Conexión Cableada** | Versión con sufijo **E**: Conector RJ45 de 100M |
| **Conexión Inalámbrica** | Versión con sufijo **W**: Wi-Fi 6 de doble banda 2.4G / 5.8G + BLE 5.4 |
| **USB / GPIO / Otros** | 1 x USB 2.0 OTG Type-C \| Headers de 2 x 14 pines (2.54 mm) \| Botones RST y BOOT \| LEDs de encendido y usuario |
| **Sistema Operativo** | Buildroot Linux / Debian |
| **Dimensiones** | 22.86 x 35.56 mm |

### Comparativa de Variantes
| Versión | Sistema Base | Micrófono / Altavoz | Ethernet | Wi-Fi 6 / BT5 |
| :--- | :---: | :---: | :---: | :---: |
| **LicheeRV-Nano-B** | Sí | Sí | No | No |
| **LicheeRV-Nano-E** | Sí | Sí | Sí | No |
| **LicheeRV-Nano-W** | Sí | Sí | No | Sí |
| **LicheeRV-Nano-WE** | Sí | Sí | Sí | Sí |

---

## 🚀 3. Guía de Réplica (Paso a Paso)

### Fase 1: Compilación Personalizada y Preparación del Sistema Operativo (Buildroot)
La imagen oficial pesa 1.2 GB e incluye software innecesario. La compilación se realiza mediante un contenedor Docker para aislar dependencias:

1. **Descarga del código fuente y creación del contenedor Docker:**
   ```bash
   git clone https://github.com/sipeed/LicheeRV-Nano-Build --depth=1
   cd LicheeRV-Nano-Build
   git clone https://github.com/sophgo/host-tools --depth=1
   cd host/ubuntu
   docker build -t licheervnano-build-ubuntu .
   ```

2. **Inicio del contenedor:**
   ```bash
   docker run -v /path/to/LicheeRV-Nano-Build/:/workspace -it licheervnano-build-ubuntu /bin/bash
   ```
   *(Reemplaza `/path/to/LicheeRV-Nano-Build/` con la ruta absoluta de tu equipo anfitrión).*

3. **Configuración y ensamblaje de la imagen:**
   Dentro del contenedor, ejecuta:
   ```bash
   cd /workspace
   source build/cvisetup.sh
   defconfig sg2002_licheervnano_sd
   git config --global --add safe.directory /workspace
   build_all
   ```

4. **Flasheo de la imagen en la tarjeta MicroSD:**
   ```bash
   xzcat buildroot_licheervnano.img.xz | sudo dd of=/dev/sdX conv=sync status=progress
   ```
   *(Asegúrate de reemplazar `/dev/sdX` con el identificador correcto de tu tarjeta MicroSD)*.

5. **Configuración del Wi-Fi (Versiones W / WE):**
   Monta la partición `rootfs` de la tarjeta SD y configura las credenciales de red editando o creando el archivo `/etc/wpa_supplicant.conf`:
   ```text
   ctrl_interface=/var/run/wpa_supplicant
   ap_scan=1
   network={
       ssid="TU_RED_WIFI"
       psk="TU_CONTRASEÑA"
   }
   ```

6. **Conexión e Inicio de Sesión:**
   Inserta la MicroSD, enciende la placa y localiza su IP en la red local mediante `nmap`:
   ```bash
   sudo nmap -sV -p 22 192.168.1.0/24
   ```
   Conéctate vía SSH (Usuario por defecto: `root`):
   ```bash
   ssh root@<IP_DE_LA_PLACA>
   ```

---

### Fase 2: Cuantización y Compilación del Modelo YOLO para la NPU
Para lograr inferencia en tiempo real en el chip SG2002:
1. Exporta tu arquitectura YOLO (ej. YOLOv8n) desde PyTorch a formato **ONNX**.
2. Emplea las herramientas de cuantización a **INT8** provistas en el SDK del fabricante para generar el archivo compatible con la NPU (`.cvimodel`).
3. Transfiere el modelo generado a la placa LicheeRV Nano mediante SCP:
   ```bash
   scp modelo_int8.cvimodel root@<IP_DE_LA_PLACA>:/root/
   ```

---

### Fase 3: Pipeline de Visión por Computadora (En Desarrollo / 60%)
- [x] Configuración de drivers del kernel para la cámara CSI.
- [x] Integración inicial de las librerías del Multimedia Framework (MMF) y CVI TDL SDK.
- [ ] Pruebas finales de estabilidad de fotogramas por segundo (FPS) bajo carga continua.
- [ ] Automatización de scripts de ejecución al arranque del sistema.

---

## 📊 4. Estado de Avance y Tareas Pendientes (Cronograma)
- **Semanas 1 - 4:** Configuración de hardware, compilación de Buildroot y conectividad Wi-Fi/SSH — **(Completado 100%)**
- **Semanas 5 - 8:** Cuantización de modelos YOLO a INT8 y pruebas de compilación cruzada con el SDK — **(Completado 100%)**
- **Semanas 9 - 12:** Desarrollo del pipeline de cámara, integración con la NPU y pruebas de rendimiento — **(En progreso - ~50%)**
- **Semanas 13 - 16:** Consolidación de la documentación final en GitHub y sustentación — **(Pendiente)**

---

## 📜 5. Licencia y Créditos
Proyecto desarrollado bajo los parámetros institucionales de la **Universidad Nacional de Colombia**. 
- **Autor:** Jesús Antonio López Trigozo
- **Director:** Carlos Iván Camargo Bareño
