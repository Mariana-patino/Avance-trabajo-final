#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <dirent.h>   // Para opendir, readdir
#include <fcntl.h>    // Para open
#include <unistd.h>   // Para read, write, close
#include <sys/stat.h> // Para stat
#include <cstring>

using namespace std;

// Función para encriptar/decriptar un buffer con Vigenère
void vigenereCipher(vector<char>& data, const string& key, bool encrypt = true) {
    size_t keyLen = key.length();
    for (size_t i = 0; i < data.size(); ++i) {
        if (encrypt) {
            data[i] = (data[i] + key[i % keyLen]) % 256;
        } else {
            data[i] = (data[i] - key[i % keyLen] + 256) % 256;
        }
    }
}

// Función para procesar un único archivo
void processFile(const string& inputPath, const string& outputPath, const string& key, bool encrypt) {
    int fd = open(inputPath.c_str(), O_RDONLY);
    if (fd < 0) {
        cerr << "Error al abrir archivo: " << inputPath << endl;
        return;
    }

    struct stat st;
    if (stat(inputPath.c_str(), &st) != 0) {
        cerr << "Error al obtener tamaño de archivo: " << inputPath << endl;
        close(fd);
        return;
    }

    vector<char> buffer(st.st_size);
    read(fd, buffer.data(), st.st_size);
    close(fd);

    // Encriptar o desencriptar
    vigenereCipher(buffer, key, encrypt);

    int fd_out = open(outputPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_out < 0) {
        cerr << "Error al crear archivo de salida: " << outputPath << endl;
        return;
    }
    write(fd_out, buffer.data(), buffer.size());
    close(fd_out);

    cout << (encrypt ? "Encriptado: " : "Desencriptado: ") << inputPath << endl;
}

// Función para procesar un directorio
void processDirectory(const string& inputDir, const string& outputDir, const string& key, bool encrypt) {
    DIR* dir = opendir(inputDir.c_str());
    if (!dir) {
        cerr << "Error al abrir directorio: " << inputDir << endl;
        return;
    }

    struct dirent* entry;
    vector<thread> threads;

    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_REG) { // Archivo regular
            string inFile = inputDir + "/" + entry->d_name;
            string outFile = outputDir + "/" + entry->d_name;
            threads.emplace_back(processFile, inFile, outFile, key, encrypt);
        }
    }

    for (auto& t : threads) t.join();
    closedir(dir);
}

int main(int argc, char* argv[]) {
    bool encrypt = false, decrypt = false;
    string inputPath, outputPath, key;

    // Parseo simple de argumentos
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "-e") encrypt = true;
        else if (arg == "-u") decrypt = true;
        else if (arg == "-i" && i + 1 < argc) inputPath = argv[++i];
        else if (arg == "-o" && i + 1 < argc) outputPath = argv[++i];
        else if (arg == "-k" && i + 1 < argc) key = argv[++i];
    }

    if ((encrypt && decrypt) || (!encrypt && !decrypt) || inputPath.empty() || outputPath.empty() || key.empty()) {
        cerr << "Uso: " << argv[0] << " [-e|-u] -i <entrada> -o <salida> -k <clave>" << endl;
        return 1;
    }

    struct stat st;
    if (stat(inputPath.c_str(), &st) != 0) {
        cerr << "Ruta de entrada no válida" << endl;
        return 1;
    }

    if (st.st_mode & S_IFDIR) {
        processDirectory(inputPath, outputPath, key, encrypt);
    } else {
        processFile(inputPath, outputPath, key, encrypt);
    }

    return 0;
}
