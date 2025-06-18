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

### 3. Formulasi Perhitungan

#### 3.1 Konversi Tegangan

$$
V_\text{probe} = \text{ADC} \times \frac{6.144}{32\,767}\quad\text{(Volt)}
$$

#### 3.2 Perkiraan pH Mentah

$$
pH_\text{raw} = 3.5 \times V_\text{probe}
$$

#### 3.3 Kalibrasi Regresi Linier

Untuk 2 titik buffer:

$$
m = \frac{pH_2 - pH_1}{V_2 - V_1},\qquad  
c = pH_1 - m \cdot V_1
$$

Untuk ≥3 titik, gunakan metode kuadrat terkecil:

$$
m = \frac{\sum(V_i-\bar{V})(pH_i-\bar{pH})}{\sum(V_i-\bar{V})^2},\qquad
c = \bar{pH} - m \bar{V}
$$

Nilai akhir:

$$
pH_\text{cal} = m \times V_\text{probe} + c
$$

Clamp output: $0 \le pH \le 14$

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

1. **Persiapan**

   * Bilas probe → air deionisasi → tisu kering
   * Pastikan suhu sekitar 25 °C
2. **Pengambilan Data**

   | Buffer | Target pH | Catat Tegangan $V$ |
   | ------ | --------- | ------------------ |
   | A      | 4.01      | V₁ = \_\_\_\_ V    |
   | B      | 6.86      | V₂ = \_\_\_\_ V    |
   | C      | 9.18      | V₃ = \_\_\_\_ V    |
3. **Hitung Regresi Linier**
   Gunakan spreadsheet atau kalkulator regresi.
4. **Kirim Payload Kalibrasi MQTT** (lihat §5.2)
5. **Verifikasi**

   * Periksa ACK diterima
   * Baca pH buffer mendekati ±0.05 pH

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
