# Tugas Kecil 2 | Strategi Algoritma (IF2211)

Implementasi algoritma kompresi gambar menggunakan struktur data quadtree sebagai bagian dari Tugas Kecil 2 mata kuliah IF2211 Strategi Algoritma. Quadtree merupakan struktur pohon dimana setiap node memiliki maksimal empat anak, digunakan untuk membagi gambar menjadi blok-blok dengan pendekatan divide and conquer.

## _Divide and Conquer_

Program ini bekerja dengan membagi gambar menjadi empat blok secara rekursif. Proses pembagian akan berhenti ketika:

1. Blok dapat direpresentasikan dengan warna rata-ratanya (error di bawah threshold)
2. Ukuran blok mencapai batas minimum yang ditentukan

Struktur quadtree digunakan untuk merepresentasikan pembagian ini, dimana node daun menyimpan blok yang sudah dikompresi.

## Program Requirements

Program menggunakan bahasa C++, sehingga diperlukan hal berikut.

1. Compiler C++ (gcc, g++)

## Build and Run Program

Berikut langkah-langkah untuk menjalankan program.

1. Clone repository ini.

   ```sh
   git clone https://github.com/adielrum/Tucil2_10123004
   ```

2. Navigate to `Tucil2_10123004` directory.

   ```sh
   cd Tucil2_10123004
   ```

3. Compile C++ files to `bin`.

   ```sh
   g++ -std=c++17 src/quadtree_compression.cpp -o bin/quadtree_compression
   ```

4. Execute `quadtree_compression`.

   ```sh
   ./bin/quadtree_compression
   ```

Folder juga dapat dibuka dengan IDE sebagai project, lalu file `quadtree_compression.exe` dapat di-run secara langsung.

## Struktur Program

Berikut struktur dari program ini.

```
├── bin/
│   ├── quadtree_compression.exe
├── doc/
│   └── Tucil2_K1_10123004_Adiel Rum.pdf
├── src/
│   ├── quadtree_compression.cpp
│   ├── gif.h
│   ├── stb_image.h
│   ├── stb_image_write.h
├── test/
│   ├── original/
|   │   ├── test1.jpg
|   │   ├── test2.jpg
|   │   ├── test3.jpg
|   │   ├── test4.jpg
|   │   ├── test5.jpg
|   │   ├── test6.jpg
|   │   └── test7.jpg
│   ├── compressed/
|   │   ├── hasil1.jpg
|   │   ├── hasil1_gif.gif
|   │   ├── hasil2.jpg
|   │   ├── hasil2_gif.gif
|   │   ├── hasil3.jpg
|   │   ├── hasil3_gif.gif
|   │   ├── hasil4.jpg
|   │   ├── hasil4_gif.gif
|   │   ├── hasil5.jpg
|   │   ├── hasil5_gif.gif
|   │   ├── hasil6.jpg
|   │   ├── hasil6_gif.gif
|   │   ├── hasil7.jpg
|   │   └── hasil7_gif.gif
└── README.md
```

## How It Works

Program kompresi gambar ini berupa CLI. Setelah menjalankan program, berikut masukan yang diperlukan.

1. Alamat dari gambar yang ingin dikompresi.
2. Metode perhitungan galat.
3. Ambang batas nilai galat untuk pembagian.
4. Ukuran minimum blok.
5. (Opsional) Target persentase kompresi.
6. Alamat dari gambar hasil kompresi.
7. (Opsional) Alamat dari GIF proses kompresi.

Lalu, program akan menghasilkan keluaran berikut.

1. Waktu eksekusi kompresi (dalam detik).
2. Ukuran gambar sebelum kompresi (dalam byte).
3. Ukuran gambar sesudah kompresi (dalam bute).
4. Persentase kompresi yang tercapai.
5. Kedalaman pohon quadtree.
6. Jumlah total node.
7. Gambar hasil kompresi pada alamat yang ditentukan.
8. (Opsional) GIF proses kompresi pada alamat yang ditentukan.

Berikut contoh kompresi yang dilakukan program.

<p align="center">
   <img src="https://github.com/adielrum/Tucil2_10123004/blob/main/test/original/test1.jpg" width="49%">
   <img src="https://github.com/adielrum/Tucil2_10123004/blob/main/test/compressed/hasil1.jpg" width="49%"> 
</p>

## Author

[Adiel Rum](https://github.com/adielrum) (10123004)