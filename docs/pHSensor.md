## pH Sensor

### 1. Hardware Layer  

| Item                     | Nilai / Pin        | Catatan                                          |
|--------------------------|--------------------|--------------------------------------------------|
| Probe                    | Elektroda kaca pH (0–14) | Memerlukan modul konverter impedansi       |
| ADC                      | ADS1115, Gain = ±6.144 V | Resolusi 0.1875 mV / LSB                     |
| Channel                  | 0 (`PH_CHANNEL`)   | Input single-ended                              |
| Tegangan referensi (ADS)| 3.3 V (I²C internal)| Default dari ADS1115                            |
| Tegangan output probe    | 0 V (≈pH 0) – 3 V (≈pH 14) | Bergantung slope probe                    |
| Pin I²C ESP32-S3         | SDA = GPIO 21, SCL = GPIO 22 | Sesuai default pinout                       |

---

### 2. Firmware Data-Path  

```mermaid
flowchart TD
    START[Start Sensor Read Cycle] --> READ_ADC["ads.readADC_SingleEnded(0)"]
    READ_ADC --> CHECK_ADC{ADC ≠ 0?}
    CHECK_ADC -- No  --> RETRY["Retry (≤ 100 ms)\nyield() + delay(1 ms)"] --> READ_ADC
    CHECK_ADC -- Yes --> CALC_VOLT["V = ADC × 6.144 / 32767"]
    CALC_VOLT --> CALC_RAW["raw_pH = V × 3.5"]
    CALC_RAW --> IS_CAL{Calibrated?}
    IS_CAL -- Yes --> CALC_CAL["cal_pH = m × V + c"]
    IS_CAL -- No  --> USE_RAW["cal_pH = raw_pH"]
    CALC_CAL --> CLAMP["Clamp 0 ≤ pH ≤ 14"]
    USE_RAW  --> CLAMP
    CLAMP --> PUBLISH["Publish to MQTT payload"]
````

---

### 3. Formulasi Perhitungan(berdasarkan kode)

#### 3.1 Konversi Tegangan

```cpp
*volt_V = adc * (6.144f / 32767.0f);
```

> **Note:**
>
> * ADC dari ADS1115 bekerja pada ±6.144 V dengan 15-bit resolusi signed (range -32768 s.d. 32767).
> * Karena hanya menggunakan single-ended input (positif terhadap GND), nilai `adc` akan berada pada 0 s.d. 32767.
> * Faktor konversi menjadi voltase: `6.144 / 32767 ≈ 0.0001875 V/LSB`.

---

#### 3.2 Perkiraan pH Mentah

```cpp
*raw_pH = (*volt_V) * 3.5f;
```

> **Note:**
>
> * Rasio 3.5 pH/V merupakan pendekatan umum untuk sensor pH dengan slope ±59.16 mV/pH.
> * Tidak akurat tanpa kalibrasi, namun cukup untuk estimasi awal.

---

#### 3.3 Kalibrasi Regresi Linier

```cpp
*cal_pH = (ph_regression_m * (*volt_V)) + ph_regression_c;
```

> **Note:**
>
> * Jika `ph_calibrated == true`, maka digunakan parameter regresi linier `m` dan `c` dari NVS.
> * Ini diperoleh dari pengguna berdasarkan pembacaan tegangan terhadap larutan buffer pH.
> * Metode umum:
>
>   * **2 titik**:
>
>     ```math
>     m = (pH₂ - pH₁) / (V₂ - V₁)  
>     c = pH₁ - m × V₁
>     ```
>   * **≥3 titik (least-squares)**:
>
>     ```math
>     m = Σ[(Vᵢ−𝑉̄)(pHᵢ−𝑝𝐻̄)] / Σ[(Vᵢ−𝑉̄)²]  
>     c = 𝑝𝐻̄ − m × 𝑉̄
>     ```

---

#### 3.4 Clamp Output

```cpp
*raw_pH = constrain(*raw_pH, 0.0f, 14.0f);
*cal_pH = constrain(*cal_pH, 0.0f, 14.0f);
```

> **Note:**
>
> * Mencegah nilai tidak realistis akibat noise, ADC overflow, atau kegagalan sensor.
> * Nilai pH dalam air umumnya berada pada rentang 0–14.

---

### 4. EEPROM / NVS

| Namespace  | Key      | Tipe  | Default |
| ---------- | -------- | ----- | ------- |
| `elsa_cal` | `ph_m`   | float | 1.0     |
|            | `ph_c`   | float | 0.0     |
|            | `ph_cal` | bool  | false   |

Dimuat saat boot melalui `loadCalibrationData()`, disimpan via `saveCalibration()`.

---

### 5. Integrasi MQTT

#### 5.1 Telemetri

Topik `elsaiot/<device_id>/data`

```json
{
  "ph":{
    "raw":7.21,
    "voltage":2.06,
    "calibrated":6.88,
    "calibrated_ok":true,
    "status":"GOOD"
  }
}
```

#### 5.2 Perintah Kalibrasi

Topik `elsaiot/<device_id>/calibrate`

```json
{
  "ph":{"m":-5.50,"c":21.90}
}
```

Efek:

1. Simpan `m` & `c` ke NVS.
2. Set `ph_calibrated = true`.
3. Kirim ACK ke `elsaiot/<device_id>/ack/calibrate`.

#### 5.3 Update Ambang Batas

Topik `elsaiot/<device_id>/offset`

```json
{
  "threshold":{ "ph_good":6.4, "ph_bad":8.2 }
}
```

Firmware memperbarui NVS dan mengirim ACK.

---

### 6. Prosedur Kalibrasi Lapangan

---

#### 6.1 Untuk End User (Antarmuka Web)

1. **Persiapan**
   - Bilas probe dengan air deionisasi, keringkan dengan tisu bersih.
   - Pastikan suhu sekitar 25 °C (disarankan suhu ruangan stabil).

2. **Langkah Kalibrasi Otomatis**
   - Buka halaman web antarmuka perangkat.
   - Pilih menu **Kalibrasi pH**.
   - Siapkan larutan buffer pH: **4.01**, **6.86**, dan **9.18**.
   - Ikuti urutan langkah:

     | Langkah | Aksi |
     |--------|------|
     | 1.     | Celupkan sensor ke larutan pH 4.01 |
     | 2.     | Klik tombol **Konfirmasi Buffer 4.01** |
     | 3.     | Sistem merekam otomatis `V₁` dari pembacaan real-time |
     | 4.     | Lanjut ke buffer pH 6.86 → Konfirmasi |
     | 5.     | Lanjut ke buffer pH 9.18 → Konfirmasi |
     | 6.     | Sistem backend hitung regresi linier |
     | 7.     | Sistem kirim payload MQTT ke perangkat |

3. **Verifikasi**
   - Perangkat mengirim ACK melalui MQTT (`/ack/calibrate`)
   - Hasil bacaan pH stabil dan sesuai ±0.05 dari buffer saat dites ulang

---


### 6.2 Untuk Sistem / Backend (Otomatisasi Teknis)

#### A. Konteks Operasional
- Perangkat terus-menerus mengirim data real-time via MQTT ke:
```

elsaiot/\<device\_id>/data

````
- Payload berisi tegangan pH (`ph.voltage`) setiap siklus pembacaan.
- Backend bertugas mengumpulkan nilai tegangan saat user mengonfirmasi pH buffer tertentu via UI.

---

#### B. Pengumpulan Sampel
Saat user menekan tombol "Konfirmasi Buffer x.xx", sistem backend akan:
1. Baca payload terakhir dari `ph.voltage` (misal 2.034 V)
2. Simpan pasangan data:
 ```json
 {
   "ph": 6.86,
   "voltage": 2.034
 }
````

Lakukan hal yang sama untuk 2–3 titik kalibrasi (minimal 2):

```json
[
  { "ph": 4.01, "voltage": 2.890 },
  { "ph": 6.86, "voltage": 2.034 },
  { "ph": 9.18, "voltage": 1.481 }
]
```

---

#### C. Perhitungan Regresi Linier

**Jika 2 titik digunakan:**

```math
m = (pH₂ - pH₁) / (V₂ - V₁)
c = pH₁ - m × V₁
```

**Contoh:**

```text
pH₁ = 4.01, V₁ = 2.890
pH₂ = 6.86, V₂ = 2.034

m = (6.86 - 4.01) / (2.034 - 2.890) = 2.85 / (-0.856) ≈ -3.33
c = 4.01 - (-3.33 × 2.890) ≈ 4.01 + 9.62 ≈ 13.63
```

**Jika 3 titik digunakan:**
Gunakan metode kuadrat terkecil (least squares):

Rumus:

```math
m = Σ[(Vᵢ - 𝑉̄)(pHᵢ - 𝑝𝐻̄)] / Σ[(Vᵢ - 𝑉̄)²]
c = 𝑝𝐻̄ - m × 𝑉̄
```

Langkah:

1. Hitung rata-rata voltage dan pH:

   ```text
   𝑉̄ = (V₁ + V₂ + V₃)/3
   𝑝𝐻̄ = (pH₁ + pH₂ + pH₃)/3
   ```
2. Hitung `m`, lalu `c`

**Contoh:**

```text
Data:
V = [2.890, 2.034, 1.481]
pH = [4.01, 6.86, 9.18]

V̄ = 2.135, pH̄ = 6.683
Σ(Vi−V̄)(pHi−pH̄) = -3.981
Σ(Vi−V̄)² = 1.003
m = -3.981 / 1.003 ≈ -3.97
c = 6.683 - (-3.97 × 2.135) ≈ 15.16
```

---

#### D. Publish Kalibrasi ke Perangkat

Setelah `m` dan `c` dihitung, backend publish ke topik:

```
elsaiot/<device_id>/calibrate
```

Dengan payload:

```json
{
  "ph": {
    "m": -3.97,
    "c": 15.16
  }
}
```

---

#### E. Validasi

1. Perangkat menyimpan ke NVS dan mengatur `ph_calibrated = true`
2. ACK akan dikirim ke:

   ```
   elsaiot/<device_id>/ack/calibrate
   ```
3. Backend bisa membandingkan hasil `ph.calibrated` dengan target buffer untuk validasi deviasi:

   ```text
   error = |measured_pH - target_pH| ≤ 0.05
   ```

Jika valid, backend tandai kalibrasi sukses.

---

### 7. Logika Ambang Batas

$$
ph\_good \le pH_\text{cal} \le ph\_bad
$$

### 8. Warna Status

| Status | Warna | RGB565 |
| ------ | ----- | ------ |
| GOOD   | Hijau | 0x07E0 |
| BAD    | Merah | 0xF800 |

Fungsi `getPHColor()` akan memberi kode warna untuk tampilan.

---

### 9. Penanganan Error

| Kasus                                 | Tindakan                                       |
| ------------------------------------- | ---------------------------------------------- |
| ADC = 0 selama 100ms                  | Skip baca, log warning                         |
| Tegangan tidak valid (<0 atau >6.144) | Dianggap tidak valid (`isPHReadingValid`)      |
| Param regresi tidak tersedia          | Gunakan `pH_raw`, status `calibrated_ok=false` |
| Probe terputus                        | Tegangan melayang → status menjadi BAD         |

---

### 10. Referensi Simbol

| Simbol   | Deskripsi                  | Satuan |
| -------- | -------------------------- | ------ |
| `V`      | Tegangan hasil pembacaan   | Volt   |
| `m`      | Slope regresi linier       | pH/V   |
| `c`      | Intersep regresi           | pH     |
| `pH_raw` | Nilai pH mentah            | pH     |
| `pH_cal` | Nilai pH setelah kalibrasi | pH     |
