#include <bits/stdc++.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "gif.h" 

using namespace std;

// ================= Konfigurasi =================
struct Config {
    int error_method;          // 1 = Varians, 2 = MAD, 3 = MaksDiff, 4 = SSIM
    float error_threshold;     // nilai batas kesalahan
    int min_block_size;        // Ukuran blok minimum sebelum memaksa menjadi daun
    float target_compression;  // Rasio kompresi target (ukuran_asli / perkiraan_ukuran_terkompresi)
                               // 0 untuk menonaktifkan penyesuaian otomatis
};

// --- Catatan parameter yang baik untuk nilai batas kesalahan ---
// Varians: Gunakan angka kecil (misalnya 10 sampai 50)
// MAD: Nilai tipikal sekitar 5 sampai 20
// MaksDiff: Batas yang berguna sekitar 20 sampai 100
// SSIM: Karena nilainya dalam (0,1], ambang batas seperti 0,90 sampai 0,99 bekerja dengan baik

// Struktur RGB
struct RGB {
    int r, g, b;
    RGB() : r(0), g(0), b(0) {}
    RGB(int r, int g, int b) : r(r), g(g), b(b) {}
    bool operator==(const RGB& other) const { return r == other.r && g == other.g && b == other.b; }
    bool operator!=(const RGB& other) const { return !(*this == other); }
};

// Struktur Box untuk merepresentasikan area persegi panjang dalam gambar
struct Box {
    int left, top, right, bottom;
    Box() : left(0), top(0), right(0), bottom(0) {}
    Box(int l, int t, int r, int b) : left(l), top(t), right(r), bottom(b) {}
    int width() const { return right - left; }
    int height() const { return bottom - top; }
};

// Metrik Kesalahan untuk pelaporan
namespace ErrorMetrics {
    double calculate_mse(unsigned char* original, unsigned char* compressed, int width, int height, int channels) {
        double sum = 0.0;
        int pixels = width * height;
        int total_samples = pixels * channels;
        for (int i = 0; i < total_samples; i++) {
            double diff = original[i] - compressed[i];
            sum += diff * diff;
        }
        return sum / total_samples;
    }
    
    double calculate_psnr(double mse) {
        if (mse < 0.0001) return 100.0;
        return 10.0 * log10(255.0 * 255.0 / mse);
    }
    
    double calculate_ssim(unsigned char* original, unsigned char* compressed, int width, int height, int channels) {
        double mse = calculate_mse(original, compressed, width, height, channels);
        return 1.0 / (1.0 + mse);
    }
    
    double calculate_compression_ratio(int original_size, int compressed_size) {
        return static_cast<double>(original_size) / compressed_size;
    }
};

// ================= Kelas Node Quadtree =================
class QuadtreeNode {
public:
    Box box;
    int depth;
    RGB color;
    // 'error' dihitung menggunakan salah satu dari empat metode
    double error;
    int width, height;
    bool leaf;
    vector<shared_ptr<QuadtreeNode>> children; // Urutan: kiri-atas, kanan-atas, kiri-bawah, kanan-bawah

    // Konstruktor: menghitung warna rata-rata dan kesalahan
    QuadtreeNode(unsigned char* img_data, int img_width, int img_height, int channels,
                 const Box& box, int depth, const Config &config)
        : box(box), depth(depth), leaf(false)
    {
        width = box.width();
        height = box.height();

        // Hitung warna rata-rata
        long sum_r = 0, sum_g = 0, sum_b = 0;
        int count = 0;
        for (int y = box.top; y < box.bottom; y++) {
            for (int x = box.left; x < box.right; x++) {
                int idx = (y * img_width + x) * channels;
                sum_r += img_data[idx];
                sum_g += img_data[idx + 1];
                sum_b += img_data[idx + 2];
                count++;
            }
        }
        if (count == 0) count = 1;
        color = RGB(static_cast<int>(sum_r / count),
                    static_cast<int>(sum_g / count),
                    static_cast<int>(sum_b / count));

        // Untuk setiap piksel, hitung perbedaan dari rata-rata
        double sumSq = 0.0;  // Untuk Varians (perbedaan kuadrat rata-rata)
        double sumAbs = 0.0; // Untuk MAD (deviasi absolut rata-rata)
        double maxDiff = 0.0; // Untuk MaksDiff
        for (int y = box.top; y < box.bottom; y++) {
            for (int x = box.left; x < box.right; x++) {
                int idx = (y * img_width + x) * channels;
                double dr = abs(img_data[idx]     - color.r);
                double dg = abs(img_data[idx + 1] - color.g);
                double db = abs(img_data[idx + 2] - color.b);
                sumSq += (dr * dr + dg * dg + db * db);
                sumAbs += (dr + dg + db);
                maxDiff = max({ maxDiff, dr, dg, db });
            }
        }
        double variance = sumSq / (count * 3);
        double mad = sumAbs / (count * 3);

        // Hitung kesalahan berdasarkan metode yang dipilih
        if (config.error_method == 1) {
            error = variance;
        } else if (config.error_method == 2) {
            error = mad;
        } else if (config.error_method == 3) {
            error = maxDiff;
        } else if (config.error_method == 4) {
            error = 1.0 / (1.0 + variance);
        } else {
            error = variance;  // Default
        }
    }
    
    // Bagi node menjadi empat kuadran
    void split(unsigned char* img_data, int img_width, int img_height, int channels, const Config &config) {
        int midX = box.left + width / 2;
        int midY = box.top + height / 2;
        children.push_back(make_shared<QuadtreeNode>(img_data, img_width, img_height, channels,
                                        Box(box.left, box.top, midX, midY), depth + 1, config));
        children.push_back(make_shared<QuadtreeNode>(img_data, img_width, img_height, channels,
                                        Box(midX, box.top, box.right, midY), depth + 1, config));
        children.push_back(make_shared<QuadtreeNode>(img_data, img_width, img_height, channels,
                                        Box(box.left, midY, midX, box.bottom), depth + 1, config));
        children.push_back(make_shared<QuadtreeNode>(img_data, img_width, img_height, channels,
                                        Box(midX, midY, box.right, box.bottom), depth + 1, config));
    }
};

// ================= Kelas Quadtree =================
class Quadtree {
private:
    shared_ptr<QuadtreeNode> root;
    int width, height;
    int max_depth;
    unsigned char* original_data;
    unsigned char* img_data; // pointer ke data gambar asli
    int channels;
    Config config;
    
    // Membangun quadtree secara rekursif
    void build_tree(unsigned char* img_data, int img_width, int img_height, int channels, 
                    shared_ptr<QuadtreeNode> node) {
        // Berhenti membagi jika blok lebih kecil dari minimum yang diizinkan
        if (node->width <= config.min_block_size || node->height <= config.min_block_size) {
            node->leaf = true;
            if (node->depth > max_depth)
                max_depth = node->depth;
            return;
        }
        
        // Tentukan kondisi berhenti berdasarkan metode kesalahan
        bool stopSplit = false;
        if (config.error_method == 4) {
            // Untuk SSIM, semakin tinggi semakin baik
            if (node->error >= config.error_threshold)
                stopSplit = true;
        } else {
            // Untuk Varians, MAD, dan MaksDiff lebih rendah lebih baik
            if (node->error <= config.error_threshold)
                stopSplit = true;
        }
        
        if (stopSplit) {
            node->leaf = true;
            if (node->depth > max_depth)
                max_depth = node->depth;
            return;
        }
        
        // Jika tidak, bagi node
        node->split(img_data, img_width, img_height, channels, config);
        for (auto &child : node->children) {
            build_tree(img_data, img_width, img_height, channels, child);
        }
    }
    
    // Kumpulkan node daun pada atau di bawah kedalaman tertentu
    void get_leaf_nodes_recursion(shared_ptr<QuadtreeNode> node, int target_depth,
                                  vector<shared_ptr<QuadtreeNode>> &result) {
        if (node->leaf || node->depth == target_depth) {
            result.push_back(node);
        } else {
            for (auto &child : node->children)
                get_leaf_nodes_recursion(child, target_depth, result);
        }
    }
    
    // Hitung semua node dalam pohon
    int count_all_nodes(shared_ptr<QuadtreeNode> node) {
        int count = 1;
        for (auto &child : node->children)
            count += count_all_nodes(child);
        return count;
    }
    
    // Buat buffer gambar yang mewakili perkiraan quadtree pada kedalaman tertentu
    unsigned char* create_image_from_depth(int depth) {
        unsigned char* output_data = new unsigned char[width * height * 3];
        fill(output_data, output_data + width * height * 3, 0);
        vector<shared_ptr<QuadtreeNode>> leaf_nodes;
        get_leaf_nodes_recursion(root, depth, leaf_nodes);
        for (const auto &node : leaf_nodes) {
            for (int y = node->box.top; y < node->box.bottom; y++) {
                for (int x = node->box.left; x < node->box.right; x++) {
                    int idx = (y * width + x) * 3;
                    output_data[idx] = node->color.r;
                    output_data[idx + 1] = node->color.g;
                    output_data[idx + 2] = node->color.b;
                }
            }
        }
        return output_data;
    }
    
public:
    // Konstruktor membangun quadtree dari data gambar dengan konfigurasi yang diberikan
    Quadtree(unsigned char* img_data, int img_width, int img_height, int channels, const Config &cfg)
        : width(img_width), height(img_height), channels(channels), config(cfg), max_depth(0)
    {
        original_data = new unsigned char[width * height * channels];
        copy(img_data, img_data + width * height * channels, original_data);
        this->img_data = img_data;
        Box full_img(0, 0, width, height);
        root = make_shared<QuadtreeNode>(img_data, img_width, img_height, channels, full_img, 0, config);
        build_tree(img_data, img_width, img_height, channels, root);
    }
    
    ~Quadtree() { delete[] original_data; }
    
    vector<shared_ptr<QuadtreeNode>> get_leaf_nodes(int depth) {
        if (depth > max_depth)
            throw invalid_argument("Kedalaman yang diminta melebihi kedalaman pohon");
        vector<shared_ptr<QuadtreeNode>> leaf_nodes;
        get_leaf_nodes_recursion(root, depth, leaf_nodes);
        return leaf_nodes;
    }
    
    // Render dan simpan gambar terkompresi pada kedalaman tertentu
    void render_at_depth(int depth, const string &output_filename) {
        if (depth > max_depth)
            throw invalid_argument("Kedalaman yang diminta melebihi kedalaman pohon");
        unsigned char* output_data = create_image_from_depth(depth);
        stbi_write_png(output_filename.c_str(), width, height, 3, output_data, width * 3);
        
        // Hitung metrik kesalahan gambar penuh untuk pelaporan
        double mse = ErrorMetrics::calculate_mse(original_data, output_data, width, height, 3);
        double psnr = ErrorMetrics::calculate_psnr(mse);
        double ssim = ErrorMetrics::calculate_ssim(original_data, output_data, width, height, 3);
        
        int total_leaf_nodes = get_leaf_nodes(depth).size();
        int original_size = width * height * 3;
        int estimated_size = total_leaf_nodes * (sizeof(RGB) + sizeof(Box));
        double comp_ratio = ErrorMetrics::calculate_compression_ratio(original_size, estimated_size);
        
        cout << "\n--- Metrik kesalahan pada kedalaman " << depth << " ---\n";
        cout << "Menggunakan metode kesalahan: ";
        if (config.error_method == 1) cout << "Varians";
        else if (config.error_method == 2) cout << "MAD";
        else if (config.error_method == 3) cout << "MaksDiff";
        else if (config.error_method == 4) cout << "SSIM";
        cout << "\nMSE gambar penuh: " << mse;
        cout << "\nPSNR gambar penuh: " << psnr << " dB";
        cout << "\nSSIM gambar penuh: " << ssim;
        cout << "\nPerkiraan rasio kompresi: " << comp_ratio << ":1";
        cout << "\nTotal node daun: " << total_leaf_nodes << "\n";
        
        delete[] output_data;
    }
    
    // Buat GIF yang menunjukkan proses kompresi
    void create_gif(const string &filename, int delay_ms = 100) {
        GifWriter g;
        GifBegin(&g, filename.c_str(), width, height, delay_ms / 10);
        for (int d = 0; d <= max_depth; d++) {
            unsigned char* frame = create_image_from_depth(d);
            uint8_t* rgba = new uint8_t[width * height * 4];
            for (int i = 0; i < width * height; i++) {
                rgba[i * 4 + 0] = frame[i * 3 + 0];
                rgba[i * 4 + 1] = frame[i * 3 + 1];
                rgba[i * 4 + 2] = frame[i * 3 + 2];
                rgba[i * 4 + 3] = 255;
            }
            GifWriteFrame(&g, rgba, width, height, delay_ms / 10);
            delete[] rgba;
            delete[] frame;
        }
        unsigned char* final_frame = create_image_from_depth(max_depth);
        uint8_t* final_rgba = new uint8_t[width * height * 4];
        for (int i = 0; i < width * height; i++) {
            final_rgba[i * 4 + 0] = final_frame[i * 3 + 0];
            final_rgba[i * 4 + 1] = final_frame[i * 3 + 1];
            final_rgba[i * 4 + 2] = final_frame[i * 3 + 2];
            final_rgba[i * 4 + 3] = 255;
        }
        for (int i = 0; i < 4; i++)
            GifWriteFrame(&g, final_rgba, width, height, delay_ms / 10);
        GifEnd(&g);
        delete[] final_rgba;
        delete[] final_frame;
        cout << "GIF telah dibuat di " << filename << "\n";
    }
    
    
    int get_max_depth() const { return max_depth; }
    int get_total_nodes() { return count_all_nodes(root); }
};

// ================= Fungsi Utama =================
int main() {
    // 1. Path absolut gambar input
    string input_image_path;
    cout << "Masukkan path absolut gambar yang akan dikompresi: ";
    getline(cin, input_image_path);
    
    // 2. Metode pengukuran kesalahan
    int method;
    cout << "Masukkan metode pengukuran kesalahan (1 = Varians, 2 = MAD, 3 = MaksDiff, 4 = SSIM): ";
    cin >> method;
    
    // 3. Ambang batas kesalahan
    float threshold;
    cout << "Masukkan ambang batas kesalahan:\n";
    cout << " - Untuk Varians, coba nilai antara 10 dan 50.\n";
    cout << " - Untuk MAD, coba nilai antara 5 dan 20.\n";
    cout << " - Untuk MaksDiff, coba nilai antara 20 dan 100.\n";
    cout << " - Untuk SSIM, gunakan nilai antara 0 dan 1 (misal, 0,90 sampai 0,99).\n";
    cout << "Input Anda: ";
    cin >> threshold;
    
    // 4. Ukuran blok minimum
    int min_block;
    cout << "Masukkan ukuran blok minimum: ";
    cin >> min_block;
    
    // 5. Rasio kompresi target
    float targetCompression;
    cout << "Masukkan rasio kompresi target (misal, 2.0 untuk kompresi 2:1, 0 untuk menonaktifkan penyesuaian otomatis): ";
    cin >> targetCompression;
    cin.ignore();  // Bersihkan newline
    
    // 6. Path absolut gambar terkompresi output
    string output_image_path;
    cout << "Masukkan path absolut untuk output gambar terkompresi: ";
    getline(cin, output_image_path);
    
    // 7. Path absolut GIF output (jika kosong, GIF dilewati)
    string output_gif_path;
    cout << "Masukkan path absolut untuk output GIF (kosongkan untuk melewati): ";
    getline(cin, output_gif_path);
    
    // Muat gambar input
    int width, height, channels;
    unsigned char* img_data = stbi_load(input_image_path.c_str(), &width, &height, &channels, 3);
    if (!img_data) {
        cerr << "Error: Tidak dapat memuat gambar " << input_image_path << "\n";
        return 1;
    }
    channels = 3;
    cout << "Gambar dimuat: " << width << "x" << height << ", " << channels << " saluran\n";
    
    // Mulai timer
    auto t_start = chrono::high_resolution_clock::now();
    
    // Siapkan konfigurasi
    Config config;
    config.error_method = method;
    config.error_threshold = threshold;
    config.min_block_size = min_block;
    config.target_compression = targetCompression;
    
    // Bangun quadtree
    Quadtree tree(img_data, width, height, channels, config);
    
    // Penyesuaian otomatis ambang batas (hanya untuk metode Varians)
    if (config.target_compression > 0 && config.error_method == 1) {
        float low = 0.0f, high = 255.0f;
        const int iterations = 15;
        for (int i = 0; i < iterations; i++) {
            config.error_threshold = (low + high) / 2.0f;
            Quadtree tempTree(img_data, width, height, channels, config);
            int leafCount = tempTree.get_leaf_nodes(tempTree.get_max_depth()).size();
            int origSize = width * height * 3;
            int estSize = leafCount * (sizeof(RGB) + sizeof(Box));
            double achievedRatio = static_cast<double>(origSize) / estSize;
            if (achievedRatio < config.target_compression)
                low = config.error_threshold;
            else
                high = config.error_threshold;
        }
        cout << "Ambang batas Varians disesuaikan otomatis menjadi: " << config.error_threshold << "\n";
        tree = Quadtree(img_data, width, height, channels, config);
    }
    
    int render_depth = tree.get_max_depth();
    
    // Render dan simpan gambar terkompresi
    tree.render_at_depth(render_depth, output_image_path);
    cout << "Gambar terkompresi disimpan di: " << output_image_path << "\n";
    
    // Buat GIF jika path disediakan
    if (!output_gif_path.empty()) {
        tree.create_gif(output_gif_path, 500);
    }
    
    int total_nodes = tree.get_total_nodes();
    int leafCount = tree.get_leaf_nodes(render_depth).size();
    int original_size = width * height * 3;
    int estimated_size = leafCount * (sizeof(RGB) + sizeof(Box));
    double comp_ratio = static_cast<double>(original_size) / estimated_size;
    double comp_percentage = (1.0 - static_cast<double>(estimated_size) / original_size) * 100.0;
    
    auto t_end = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = t_end - t_start;
    
    cout << "\n----- Laporan Kompresi -----\n";
    cout << "Waktu eksekusi: " << elapsed.count() << " detik\n";
    cout << "Ukuran gambar asli: " << original_size << " byte\n";
    cout << "Perkiraan ukuran terkompresi: " << estimated_size << " byte\n";
    cout << "Persentase kompresi: " << comp_percentage << " %\n";
    cout << "Kedalaman quadtree: " << tree.get_max_depth() << "\n";
    cout << "Jumlah total node: " << total_nodes << "\n";
    
    stbi_image_free(img_data);
    return 0;
}