
## DO Sensor

---

### 1. Hardware Layer

| Item                        | Nilai / Pin           | Catatan                                  |
|-----------------------------|------------------------|-------------------------------------------|
| Probe                       | Galvanic/optical DO    | Output analog (mV)                        |
| ADC                         | ADS1115, ±6.144 V      | Resolusi 0.1875 mV/LSB                    |
| Channel                     | 2 (`DO_CHANNEL`)       | Input single-ended                        |
| Output probe                | 0 – ~3000 mV           | Tergantung saturasi DO dan suhu          |
| Tegangan referensi (ADS)   | 3.3 V via I²C          | Default internal                          |
| Pin I²C ESP32-S3            | SDA = GPIO 21, SCL = GPIO 22 | Sesuai default pinout               |

---

### 2. Firmware Data-Path

```mermaid
flowchart TD
    START["Mulai pembacaan"] --> READ_ADC["ads.readADC_SingleEnded(2)"]
    READ_ADC --> MV["V_mV = ADC × 6.144 / 32767 × 1000"]
    MV --> RAW_DO["DO_raw = V_mV × 6.5 / 1000"]
    RAW_DO --> IS_CALIB{Kalibrasi?}

    IS_CALIB -- "Tidak" --> USE_RAW["DO_final = DO_raw"]

    IS_CALIB -- "Ya" --> SAT_TABLE["DO_sat = lookup(T)"]
    SAT_TABLE --> VSAT_MODE{Mode kalibrasi}

    VSAT_MODE -- "2 titik" --> VSAT2["Vsat = interpolasi(T)"]
    VSAT_MODE -- "1 titik" --> VSAT1["Vsat = V_cal1 + 35·(T − T_cal1)"]

    VSAT2 --> CALC
    VSAT1 --> CALC
    CALC["DO_final = V_mV × DO_sat / Vsat / 1000"] --> CLAMP["Clamp 0–20 mg/L"]

    USE_RAW --> CLAMP
    CLAMP --> STORE["lastDOmgL ← DO_final"]
    STORE --> MQTT["Publish ke topic /data"]
````

---

### 3. Formulasi Perhitungan
#### 3.1 Tegangan Sensor (mV)

```cpp
*volt_mV = adc × (6.144 / 32767.0) × 1000.0;
````

> **Penjelasan:**
>
> * ADC ADS1115 menghasilkan nilai digital 16-bit (signed), dengan rentang 0–32767 untuk single-ended input.
> * Gain diset ke ±6.144 V → skala LSB = 6.144 / 32767 V.
> * Hasil dikonversi ke **milivolt** agar sesuai dengan rumus-rumus DO berikutnya.
>
> **Formula lengkap:**
>
> $$
> V_{\text{mV}} = \text{ADC} \times \frac{6.144}{32767} \times 1000
> $$

---

#### 3.2 Nilai DO Mentah (Tanpa Kalibrasi)

```cpp
*raw_mgL = (volt_mV × 6.5) / 1000.0;
```

> **Penjelasan:**
>
> * Merupakan estimasi kasar kandungan DO dalam air (mg/L) sebelum proses kalibrasi dilakukan.
> * Faktor 6.5 diperoleh dari uji empiris sensor terhadap larutan jenuh DO (20 °C).
> * Tujuannya hanya memberikan gambaran kasar bagi pengguna sebelum kalibrasi.
>
> **Formula:**
>
> $$
> DO_{\text{raw}} = \frac{V_{\text{mV}} \times 6.5}{1000}
> $$
>
> dengan hasil dalam satuan **mg/L**.

---

#### 3.3 Tabel Lookup Saturasi DO (`DO_Table[]`)

Tabel ini digunakan untuk menghitung saturasi oksigen terlarut (DO) di udara jenuh pada berbagai suhu, dengan satuan **mg/L × 1000**.

- `DO_Table[i]` menyimpan nilai saturasi pada suhu `i` derajat Celsius.
- Nilai diperoleh dari standar kelarutan DO di udara pada tekanan atmosfer normal (1 atm).
- Index `i` pada array: suhu integer dari 0°C hingga 40°C.

> **Catatan Implementasi**:  
> Untuk suhu di luar 0–40 °C, firmware akan otomatis menahan (`constrain`) index ke batas tabel:
> - suhu < 0°C → `DO_Table[0]`
> - suhu > 40°C → `DO_Table[40]`

---

##### Format Tabel: Saturasi O₂ di Air (mg/L) × 1000

| °C | DO_Table[i] | mg/L |   | °C | DO_Table[i] | mg/L |   | °C | DO_Table[i] | mg/L |
|----|-------------|------|---|----|-------------|------|---|----|-------------|------|
| 0  | 14460       | 14.46 |   | 10 | 11260       | 11.26 |   | 20 | 9080        | 9.08  |
| 1  | 14220       | 14.22 |   | 11 | 11010       | 11.01 |   | 21 | 8900        | 8.90  |
| 2  | 13820       | 13.82 |   | 12 | 10770       | 10.77 |   | 22 | 8730        | 8.73  |
| 3  | 13440       | 13.44 |   | 13 | 10530       | 10.53 |   | 23 | 8570        | 8.57  |
| 4  | 13090       | 13.09 |   | 14 | 10300       | 10.30 |   | 24 | 8410        | 8.41  |
| 5  | 12740       | 12.74 |   | 15 | 10080       | 10.08 |   | 25 | 8250        | 8.25  |
| 6  | 12420       | 12.42 |   | 16 | 9860        | 9.86  |   | 26 | 8110        | 8.11  |
| 7  | 12110       | 12.11 |   | 17 | 9660        | 9.66  |   | 27 | 7960        | 7.96  |
| 8  | 11810       | 11.81 |   | 18 | 9460        | 9.46  |   | 28 | 7820        | 7.82  |
| 9  | 11530       | 11.53 |   | 19 | 9270        | 9.27  |   | 29 | 7690        | 7.69  |
|    |             |       |   |    |             |       |   | 30 | 7560        | 7.56  |
|    |             |       |   |    |             |       |   | 31 | 7430        | 7.43  |
|    |             |       |   |    |             |       |   | 32 | 7300        | 7.30  |
|    |             |       |   |    |             |       |   | 33 | 7180        | 7.18  |
|    |             |       |   |    |             |       |   | 34 | 7070        | 7.07  |
|    |             |       |   |    |             |       |   | 35 | 6950        | 6.95  |
|    |             |       |   |    |             |       |   | 36 | 6840        | 6.84  |
|    |             |       |   |    |             |       |   | 37 | 6730        | 6.73  |
|    |             |       |   |    |             |       |   | 38 | 6630        | 6.63  |
|    |             |       |   |    |             |       |   | 39 | 6530        | 6.53  |
|    |             |       |   |    |             |       |   | 40 | 6410        | 6.41  |

---

> **Contoh Penggunaan dalam Firmware:**
>
> ```cpp
> uint8_t idx = constrain((int)tempC, 0, 40);
> uint16_t sat = DO_Table[idx];
> ```
>
> Nilai `sat` mewakili kandungan DO jenuh (mg/L × 1000) untuk suhu tertentu yang akan digunakan dalam perhitungan mg/L aktual:
>
> ```cpp
> DO = (volt_mV × sat) / (vSat × 1000);
> ```

---

#### 3.4 Perhitungan DO Terkompensasi (Kalibrasi Aktif)

---

Jika `do_calibrated == true`, maka firmware akan menghitung nilai DO berdasarkan suhu aktual dan tegangan hasil ADC.

---

##### Mode 1: **Kalibrasi Satu Titik (Single-Point)**

```cpp
vSat = do_cal1_v + 35.0 * (temp − do_cal1_t);
````

> **Penjelasan:**
>
> * `do_cal1_v` adalah tegangan saturasi yang dibaca saat kalibrasi dilakukan pada suhu `do_cal1_t`.
> * Dihitung secara linear berdasarkan kenaikan 35 mV/°C (nilai empiris sensor).

---

##### Mode 2: **Kalibrasi Dua Titik (Two-Point)**

```cpp
vSat = ((temp − t2) * (v1 − v2) / (t1 − t2)) + v2;
```

> **Penjelasan:**
>
> * `v1`, `t1` dan `v2`, `t2` adalah pasangan tegangan dan suhu pada dua titik kalibrasi.
> * Pendekatan ini lebih akurat karena memodelkan slope sensor berdasarkan dua data aktual.

---

##### Langkah Final: Hitung DO mg/L

```cpp
mgL = (volt_mV × DO_Table[temp]) / (vSat × 1000.0);
```

> **Penjelasan:**
>
> * `volt_mV`: hasil ADC terkonversi (input sensor saat ini)
> * `DO_Table[temp]`: nilai saturasi maksimum DO (mg/L × 1000) pada suhu tersebut
> * `vSat`: tegangan referensi saturasi (berbasis kalibrasi)
> * Output dalam **mg/L**

---

#### 3.5 Pembatasan Nilai (Clamping)

```cpp
constrain(mgL, 0.0, 20.0);
```

> **Penjelasan:**
>
> * Membatasi hasil akhir agar tetap dalam rentang realistis.
> * Rentang normal DO dalam air: 0 – 20 mg/L

---

### 4. EEPROM / NVS

| Namespace  | Key         | Tipe  | Default |
| ---------- | ----------- | ----- | ------- |
| `elsa_cal` | `do_cal`    | bool  | false   |
|            | `do_cal1_v` | float | 1.6     |
|            | `do_cal1_t` | float | 25.0    |
|            | `do_cal2_v` | float | 1.0     |
|            | `do_cal2_t` | float | 25.0    |
|            | `do_2pt`    | bool  | false   |

---

### 5. Integrasi MQTT

---

#### 5.1 Telemetri

**Topik:**  
```

elsaiot/\<device\_id>/data

````

**Payload:**
```json
{
  "do": {
    "raw": 6.71,
    "voltage": 1460.2,
    "calibrated": 7.02,
    "calibrated_ok": true,
    "status": "GOOD"
  }
}
````

**Penjelasan Field:**

| Field           | Tipe    | Keterangan                                                         |
| --------------- | ------- | ------------------------------------------------------------------ |
| `raw`           | float   | Estimasi DO (mg/L) dari tegangan sensor tanpa kalibrasi            |
| `voltage`       | float   | Tegangan sensor dalam milivolt (mV)                                |
| `calibrated`    | float   | DO hasil kalibrasi (mg/L)                                          |
| `calibrated_ok` | boolean | `true` jika kalibrasi aktif dan `vSat > 0`                         |
| `status`        | string  | Evaluasi kondisi air (GOOD/BAD) berdasarkan threshold configurable |

---

#### 5.2 Perintah Kalibrasi

**Topik:**

```
elsaiot/<device_id>/calibrate
```

---

##### Format: Kalibrasi Satu Titik (Single-Point)

```json
{
  "do": {
    "mode": "single",
    "v1": 1450.0,
    "t1": 25.0
  }
}
```

> **Efek:**
>
> * `do_calibrated = true`
> * `do_cal1_v = 1450.0`, `do_cal1_t = 25.0`
> * Mode = Single-point (linear offset dari suhu referensi)

---

##### Format: Kalibrasi Dua Titik (Two-Point)

```json
{
  "do": {
    "mode": "double",
    "v1": 1600.0,
    "t1": 20.0,
    "v2": 1000.0,
    "t2": 30.0
  }
}
```

> **Efek:**
>
> * `do_calibrated = true`
> * Kalibrasi menggunakan interpolasi linear antara dua suhu
> * Otomatis ditransformasikan ke urutan `t1 > t2` jika perlu

---

#### 5.3 Tanggapan Firmware

**ACK dikirim ke:**

```
elsaiot/<device_id>/ack/calibrate
```

**Payload contoh:**

```json
{
  "do_ack": "ok",
  "mode": "single",
  "calibrated": true
}
```

---

### 6. Prosedur Kalibrasi Lapangan – Dissolved Oxygen (DO)

---

#### 6.1 Untuk Pengguna (UI Elsaiot)

**Tujuan:** Mengatur pembacaan sensor DO agar sesuai dengan nilai aktual larutan jenuh oksigen pada suhu tertentu, menggunakan satu atau dua titik kalibrasi.

---

##### A. Persiapan

1. Siapkan larutan jenuh DO (misalnya: air deionisasi tersaturasi oksigen).
2. Pastikan suhu larutan diketahui secara akurat (gunakan termometer eksternal).
3. Hidupkan sensor dan tunggu hingga nilai stabil (≈10 detik).

---

##### B. Langkah Kalibrasi (UI)

1. Pilih menu **Kalibrasi DO**.
2. Tentukan mode kalibrasi:
   - **Single-point** → gunakan 1 suhu (misalnya 25 °C).
   - **Two-point** → gunakan 2 suhu berbeda (misalnya 20 °C dan 30 °C).
3. Untuk tiap titik:
   - Celupkan probe ke larutan.
   - Tekan tombol “Konfirmasi Titik Kalibrasi”.
   - Sistem akan otomatis menyimpan:
     - Tegangan (`vX`) dari stream `/data`.
     - Suhu (`tX`) saat itu.
4. Setelah semua titik disimpan, backend akan mengirimkan payload ke perangkat.

---

##### C. Payload Kalibrasi MQTT

- Topik:
```

elsaiot/\<device\_id>/calibrate

````

- Format:

**Single-Point**
```json
{
"do": {
  "mode": "single",
  "v1": 1450.0,
  "t1": 25.0
}
}
````

**Two-Point**

```json
{
  "do": {
    "mode": "double",
    "v1": 1600.0,
    "t1": 20.0,
    "v2": 1000.0,
    "t2": 30.0
  }
}
```

---

##### D. Konfirmasi Kalibrasi

1. Perangkat menerima payload di `handleCalibrateMessage()`.
2. Menyimpan nilai `v1`, `t1`, `v2`, `t2` ke dalam NVS.
3. Aktifkan `do_calibrated = true` dan `do_two_point_mode` sesuai mode.
4. Kirim balasan ACK:

```json
{
  "status": {
    "do": {
      "calibrated": true,
      "mode": "single-point"
    }
  }
}
```

---

#### 6.2 Proses Backend Teknis (Elsaiot)

* Backend hanya bertugas sebagai relay UI → MQTT.
* Tidak ada perhitungan `vSat`, `DO_Table`, atau `mg/L` di backend.
* Semua logika kalibrasi berada di firmware perangkat (lihat `calcDOmgL()`).
* Fungsi backend:

  1. Ambil `voltage` dan `temperature` dari stream `/data`.
  2. Bungkus sebagai payload sesuai format.
  3. Kirim ke topik MQTT `calibrate`.
  4. Terima dan simpan ACK di log aplikasi.

---

> 🔧 Mode single-point cukup untuk sebagian besar aplikasi. Gunakan two-point hanya jika DO digunakan di suhu bervariasi ekstrem (mis. akuakultur).

### 7. Logika Ambang Batas DO

---

Nilai DO hasil kalibrasi (`do_calibrated`) akan dievaluasi terhadap dua parameter ambang batas:

```text
do_good ≤ do_calibrated ≤ do_bad
````

---

#### Penjelasan:

| Parameter | Tipe   | Fungsi                                                                     |
| --------- | ------ | -------------------------------------------------------------------------- |
| `do_good` | float  | Nilai minimum DO yang dianggap aman                                        |
| `do_bad`  | float  | Nilai maksimum DO yang dianggap aman                                       |
| `status`  | string | Akan di-set ke `"GOOD"` jika nilai berada di rentang, `"BAD"` jika di luar |

---

#### Contoh:

```json
{
  "threshold": {
    "do_good": 5.0,
    "do_bad": 12.0
  }
}
```

```json
{
  "do": {
    "calibrated": 6.8,
    "status": "GOOD"
  }
}
```

```json
{
  "do": {
    "calibrated": 3.2,
    "status": "BAD"
  }
}
```

---

Threshold ini dapat diperbarui via MQTT (`/offset`) dan disimpan dalam NVS.

---

### 8. Warna Status

| Status | Warna  | RGB565 |
| ------ | ------ | ------ |
| GOOD   | Orange | 0xFD20 |
| BAD    | Merah  | 0xF800 |

---

### 9. Penanganan Error

| Kasus                 | Tindakan                              |
| --------------------- | ------------------------------------- |
| ADC = 0               | DO = 0                                |
| vSat = 0 atau negatif | Return 0                              |
| Belum dikalibrasi     | Gunakan mode default `V × 6.5 / 1000` |

---

### 10. Referensi Simbol

| Simbol        | Deskripsi                        | Satuan     |
| ------------- | -------------------------------- | ---------- |
| `V`           | Tegangan dari sensor             | mV         |
| `T`           | Suhu air                         | °C         |
| `DO_Table[T]` | Nilai saturasi pada suhu T       | µg/L ×1000 |
| `vSat`        | Tegangan referensi pada saturasi | mV         |
| `mgL`         | Hasil akhir DO                   | mg/L       |
| `K`           | Faktor regresi (pada TDS)        | -          |

---
