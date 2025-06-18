## TDS Sensor

### 1. Hardware Layer

| Item                      | Nilai / Pin            | Catatan                                          |
|---------------------------|------------------------|--------------------------------------------------|
| Probe/Modul               | Sensor TDS analog      | Biasanya HLW8032/DFROBOT analog TDS              |
| ADC                       | ADS1115, Gain = ±6.144 V | Resolusi 0.1875 mV / LSB                       |
| Channel                   | 1 (`TDS_CHANNEL`)      | Input single-ended                              |
| Tegangan output sensor    | 0–2.3 V (umum)         | Berbanding lurus terhadap ppm                   |
| Tegangan referensi (ADS) | 3.3 V (I²C internal)   | Default dari ADS1115                            |
| Pin I²C ESP32-S3          | SDA = GPIO 21, SCL = GPIO 22 | Sesuai default pinout                     |

---

### 2. Firmware Data-Path

```mermaid
flowchart TD
    START["Mulai pembacaan"] --> READ_ADC["Baca ADC\nads.readADC_SingleEnded(1)"]
    READ_ADC --> V_CALC["Hitung tegangan\nV = ADC × 6.144 / 32767"]
    V_CALC --> RAW_TDS["TDS_mentah = V × 1000"]
    
    RAW_TDS --> IS_CALIB{Sudah kalibrasi?}
    
    IS_CALIB -- "Tidak" --> USE_RAW["TDS_akhir = TDS_mentah"]
    USE_RAW --> LIMIT
    
    IS_CALIB -- "Ya" --> TEMP_COMP["Kompensasi suhu\ncoeff = 1 + 0.02 × (T − 25)"]
    TEMP_COMP --> V_COMP["V' = V / coeff"]
    V_COMP --> POLY["f(V') = 133.42·V'^3 − 255.86·V'^2 + 857.39·V'"]
    POLY --> APPLY_K["TDS_akhir = f(V') × 0.5 × K"]
    APPLY_K --> LIMIT
    
    LIMIT["Clamp 0 – 1000 ppm"] --> STORE["Simpan ke lastTDSmgL"]
    STORE --> MQTT["Publish ke MQTT /data"]
````

---

### 3. Formulasi Perhitungan (berdasarkan kode)

---

#### 3.1 Konversi Tegangan ADC ke Voltase

```cpp
*voltage = adc_raw * (6.144 / 32767.0);
````

> **Penjelasan:**
>
> * ADC menghasilkan nilai digital 16-bit dari tegangan input (range 0–32767 untuk mode single-ended).
> * Nilai tersebut dikalikan dengan skala referensi ±6.144 V dari ADS1115 untuk mendapatkan **tegangan aktual (V)**.
> * Rumus:
>   $V = \text{ADC} \times \frac{6.144}{32767}$

---

#### 3.2 TDS Mentah (Tanpa Kalibrasi)

```cpp
*raw = *voltage * 1000.0;
```

> **Penjelasan:**
>
> * Jika perangkat **belum dikalibrasi**, maka nilai TDS hanya dihitung secara linear dari voltase.
> * Ini adalah estimasi awal dalam ppm:
>   $\text{TDS}_{\text{raw}} = V \times 1000$

---

#### 3.3 Kalibrasi TDS (Jika Sudah Teraktivasi)

```cpp
compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
compensatedVoltage = voltage / compensationCoefficient;

float tds_value = (133.42 * V'^3 - 255.86 * V'^2 + 857.39 * V') * 0.5 * k;
```

> **Langkah-langkah dan penjelasan:**
>
> 1. **Kompensasi Suhu**
>
>    * Suhu memengaruhi konduktivitas → dilakukan koreksi agar akurasi konsisten:
>
>      $$
>      \text{coeff} = 1 + 0.02 \times (T - 25)
>      $$
> 2. **Normalisasi Tegangan**
>
>    * Tegangan dikompensasi terhadap suhu:
>
>      $$
>      V' = \frac{V}{\text{coeff}}
>      $$
> 3. **Fungsi Polinomial**
>
>    * Fungsi regresi 3rd-order digunakan sesuai karakteristik sensor:
>
>      $$
>      f(V') = 133.42 × V'^3 − 255.86 × V'^2 + 857.39 × V'
>      $$
> 4. **Kalibrasi Faktor K**
>
>    * K ditentukan sekali dari larutan standar (lihat §6):
>
>      $$
>      TDS = f(V') × 0.5 × K
>      $$
>    * K memastikan pembacaan sesuai standar aktual (ppm)

---

#### 3.4 Perhitungan Nilai Kalibrasi `K`

```cpp
tds_k_value = standard_value / polinomial(V');
```

> **Penjelasan:**
>
> * Saat kalibrasi dilakukan:
>
>   * Larutan standar: misal **707 ppm**
>   * Tegangan terbaca `V` dan suhu `T`
> * `f(V')` dihitung seperti di atas, lalu:
>
>   $$
>   K = \frac{\text{standar}}{f(V') \times 0.5}
>   $$
> * Nilai `K` disimpan dan digunakan permanen oleh firmware.

---

#### 3.5 Pembatasan Nilai Output (Clamping)

```cpp
*calibrated = constrain(tds_value, 0.0, 1000.0);
```

> **Penjelasan:**
>
> * Untuk menghindari nilai tidak wajar akibat noise atau kesalahan input.
> * Nilai TDS ditahan pada rentang aman:
>
>   $$
>   0 \le \text{TDS} \le 1000\ \text{ppm}
>   $$

---

### 4. EEPROM / NVS

| Namespace  | Key       | Tipe  | Default |
| ---------- | --------- | ----- | ------- |
| `elsa_cal` | `tds_k`   | float | 1.0     |
|            | `tds_cal` | bool  | false   |

---

### 5. Integrasi MQTT

#### 5.1 Telemetri

Topic: `elsaiot/<device_id>/data`

```json
{
  "tds": {
    "raw": 674.2,
    "voltage": 2.134,
    "calibrated": 705.8,
    "calibrated_ok": true,
    "status": "GOOD"
  }
}
```

#### 5.2 Kalibrasi

Topik: `elsaiot/<device_id>/calibrate`

```json
{
  "tds": {
    "v": 2.134,
    "std": 700.0,
    "t": 25.0
  }
}
```

---

### 6. Prosedur Kalibrasi Lapangan

---

#### 6.1 Untuk End User (Via Web)

##### **Tujuan**  
Mengkalibrasi pembacaan sensor TDS berdasarkan larutan standar (misalnya 707 ppm) agar hasil pengukuran akurat dan sesuai standar teknis.

##### **Langkah-Langkah**
1. **Persiapan**
   - Siapkan larutan standar TDS (misalnya 707 ppm dari DFRobot atau pabrik terpercaya).
   - Pastikan probe bersih, bilas dengan air deionisasi lalu keringkan.
   - Tunggu hingga suhu probe dan larutan stabil (ideal 25 °C).

2. **Akses UI Kalibrasi**
   - Buka antarmuka web perangkat.
   - Navigasi ke: **Kalibrasi → Sensor TDS**.

3. **Pilih Larutan Standar**
   - Pilih nilai larutan kalibrasi dari dropdown (misalnya: **707 ppm**, **1000 ppm**, dll).
   - Nilai ini akan dikirim sebagai `std` (standard ppm).

4. **Celupkan Probe**
   - Celupkan probe ke larutan standar yang dipilih.
   - Tunggu hingga pembacaan `voltage` pada antarmuka stabil.

5. **Konfirmasi Kalibrasi**
   - Klik tombol **"Konfirmasi Kalibrasi"**.
   - Sistem akan menyimpan tegangan saat itu (`v`) dan suhu (`t`) dari sensor suhu internal.

6. **Pengiriman Otomatis**
   - Backend akan mengemas data dan mengirim ke perangkat:
     - Topik MQTT: `elsaiot/<device_id>/calibrate`
     - Payload:
       ```json
       {
         "tds": {
           "v": 2.134,
           "std": 707.0,
           "t": 25.0
         }
       }
       ```

7. **Verifikasi**
   - Setelah berhasil, UI akan menampilkan status **Kalibrasi Berhasil**.
   - Firmware akan mengirimkan ACK melalui topic `elsaiot/<device_id>/ack/calibrate`.
   - Bacaan TDS selanjutnya akan disesuaikan dengan hasil kalibrasi.

> **Catatan:**
> - Gunakan larutan kalibrasi yang valid dan masih dalam masa pakai.
> - Jangan lakukan kalibrasi jika larutan sudah terkontaminasi atau suhu fluktuatif.
> - Satu titik kalibrasi cukup untuk TDS karena fungsi kompensasi sudah polinomial.

---

#### 6.2 Backend Teknis (Perhitungan Otomatis)

##### **Trigger**
- Terpicu saat user klik **"Konfirmasi Kalibrasi"** di UI.

##### **Input**
- Tegangan `V` terbaca dari stream MQTT topic `/data`
- Suhu `T` terbaca dari field `temperature.value`
- Nilai standar dipilih user (mis. `707.0 ppm`)

##### **Langkah Perhitungan**

```math
1. Hitung koefisien kompensasi suhu:
   coeff = 1 + 0.02 × (T − 25)

2. Tegangan dikompensasi:
   V' = V / coeff

3. Hitung nilai fungsi polinomial dari tegangan V':
   f(V') = 133.42 × V'³ − 255.86 × V'² + 857.39 × V'

4. Hitung faktor kalibrasi K:
   K = std / (f(V') × 0.5)
````

##### **Contoh Nyata:**

```text
V = 2.134 V
T = 25.0 °C
std = 707.0 ppm

coeff = 1 + 0.02 × (25 − 25) = 1.0
V' = 2.134 / 1.0 = 2.134

f(V') = 133.42×(2.134)³ − 255.86×(2.134)² + 857.39×2.134
      ≈ 133.42×9.713 − 255.86×4.553 + 857.39×2.134
      ≈ 1296.2 − 1165.2 + 1829.5 ≈ 1960.5

K = 707.0 / (1960.5 × 0.5) ≈ 0.721
```

##### **Output**

* Payload MQTT ke perangkat:

  ```json
  {
    "tds": {
      "v": 2.134,
      "std": 707.0,
      "t": 25.0
    }
  }
  ```

##### **Firmware Handling**

* Fungsi `applyTDSCalibration(v, std, t)` akan:

  * Hitung `k` secara lokal dengan rumus yang sama.
  * Simpan ke NVS (`tds_k_value`, `tds_calibrated=true`).
  * Kirim ACK ke backend.

> **Catatan Backend**
>
> * Perhitungan `k` tidak perlu dikirim; hanya kirim `v`, `std`, `t`.
> * Nilai `k` dihitung ulang oleh firmware untuk validasi internal.
> * Gunakan cache data MQTT terakhir untuk `voltage` dan `temperature` snapshot.

---

### 7. Logika Ambang Batas

```text
tds_good ≤ tds_calibrated ≤ tds_bad
```

---

### 8. Warna Status

| Status | Warna | RGB565 |
| ------ | ----- | ------ |
| GOOD   | Cyan  | 0x07FF |
| BAD    | Merah | 0xF800 |

---

### 9. Penanganan Error

| Kasus                           | Tindakan                          |
| ------------------------------- | --------------------------------- |
| ADC = 0                         | Kalibrasi tetap jalan, nilai 0    |
| Tegangan sangat rendah / tinggi | Output TDS ≈ 0 atau clamp ke 1000 |
| `tds_calibrated == false`       | Raw V × 1000 digunakan            |

---

### 10. Referensi Simbol

| Simbol | Deskripsi                          | Satuan |
| ------ | ---------------------------------- | ------ |
| `V`    | Tegangan input sensor TDS          | Volt   |
| `T`    | Suhu saat pembacaan                | °C     |
| `comp` | Koefisien kompensasi suhu          | -      |
| `V'`   | Tegangan setelah dikompensasi      | Volt   |
| `K`    | Faktor kalibrasi                   | -      |
| `TDS`  | Hasil akhir pembacaan terkalibrasi | ppm    |
