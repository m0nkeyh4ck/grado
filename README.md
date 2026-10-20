# Implementación y Optimización de un Modelo YOLO en la Plataforma Embebida LicheeRV Nano (SG2002)

Repositorio oficial del trabajo de grado titulado: *"Implementación y optimización de un modelo YOLO en la plataforma embebida LicheeRV Nano (SG2002) para detección de objetos con documentación replicable en entornos educativos"*, presentado por **Jesús Antonio López Trigozo** ante la Facultad de Ingeniería de la **Universidad Nacional de Colombia (Sede Bogotá)**, bajo la dirección del profesor Carlos Iván Camargo Bareño.

> **Estado del Proyecto:** 🟢 En desarrollo (Avance actual estimado: **60%** - Entorno configurado, toolchain de NPU integrado y scripts base de inferencia implementados).

---

## 📌 1. Introducción y Justificación
El despliegue de Inteligencia Artificial en el borde (*Edge AI*) en hardware de bajo costo suele verse obstaculizado por la escasez de guías técnicas claras y reproducibles. Este proyecto aborda dicha problemática implementando un modelo YOLO optimizado sobre la placa **LicheeRV Nano (SoC SG2002)**, aprovechando su NPU nativa de 1 TOPS INT8, su interfaz de cámara CSI y conectividad Wi-Fi. El propósito central es entregar a la comunidad académica una metodología abierta, paso a paso, para facilitar el desarrollo de sistemas embebidos inteligentes.

---

## 🛠️ 2. Especificaciones Técnicas del Sistema
- **Plataforma:** LicheeRV Nano (SoC Sophgo SG2002: Dual RISC-V C906 a 1 GHz / 700 MHz, 256MB RAM DDR3).
- **Aceleración de IA:** NPU integrada con rendimiento de 1 TOPS (precisión INT8/BF16).
- **Periféricos utilizados:** Módulo de cámara CSI, antena Wi-Fi de doble banda (N2425D) y tarjeta MicroSD (Clase 10, formato MBR).
- **Software Base:** Buildroot Linux oficial o compilado a medida.

---

## 🚀 3. Guía de Réplica (Paso a Paso)

### Fase 1: Compilación Personalizada y Preparación del Sistema Operativo (Buildroot)
La imagen oficial pesa 1.2 GB e incluye software innecesario (como Python y controladores de red no utilizados) que no es práctico para las limitaciones de recursos de la placa. La compilación se realiza mediante un contenedor Docker para evitar conflictos con las dependencias del sistema anfitrión:

1. **Descarga del código fuente y creación del contenedor Docker:**
   ```bash
   git clone [https://github.com/sipeed/LicheeRV-Nano-Build](https://github.com/sipeed/LicheeRV-Nano-Build) --depth=1
   cd LicheeRV-Nano-Build
   git clone [https://github.com/sophgo/host-tools](https://github.com/sophgo/host-tools) --depth=1
   cd host/ubuntu
   docker build -t licheervnano-build-ubuntu .
