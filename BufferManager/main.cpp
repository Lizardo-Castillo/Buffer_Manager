#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <list>
#include <cstdlib>
#include <ctime>

class BufferPool {
private:
    struct Frame {
        int frameID;
        int pageID;
        bool dirtyBit;
        bool pinned;
        int pinCount;
        bool refBit;
    };

    std::vector<Frame> frames;
    std::string tablaFilename;

public:
    BufferPool() : tablaFilename("TablaBufferPool.txt") {}

    void MenuBufferPool() {
        int numFrames;
        std::cout << "Ingrese la cantidad de frames que tendra el buffer: ";
        std::cin >> numFrames;
        frames.resize(numFrames);

        for (int i = 0; i < numFrames; ++i) {
            frames[i] = {i, 0, false, false, 0, false}; // Inicializar pageID con -1 para indicar frame vacío
        }

        CrearTabla();
    }

    void CrearTabla() {
        std::ofstream tablaFile(tablaFilename);

        if (tablaFile.is_open()) {
            tablaFile << "Frame ID\tPage ID\t\tDirty Bit\tPinned\t\tPin Count\tRef Bit\n";
            for (const auto& frame : frames) {
                tablaFile << frame.frameID << "\t\t"
                          << frame.pageID << "\t\t"
                          << frame.dirtyBit << "\t\t"
                          << frame.pinned << "\t\t"
                          << frame.pinCount << "\t\t"
                          << frame.refBit << "\n";
            }
            tablaFile.close();
            std::cout << "Tabla creada exitosamente en " << tablaFilename << std::endl;
        } else {
            std::cerr << "No se pudo crear el archivo: " << tablaFilename << std::endl;
        }
    }

    void MostrarTabla() {
        std::ifstream tablaFile(tablaFilename);
        if (tablaFile.is_open()) {
            std::string line;
            while (std::getline(tablaFile, line)) {
                std::cout << line << std::endl;
            }
            tablaFile.close();
        } else {
            std::cerr << "No se pudo abrir el archivo: " << tablaFilename << std::endl;
        }
    }

    void ActualizarTabla() {
        CrearTabla();
    }

    friend class BufferManager;
};

class BufferManager {
private:
    BufferPool& bufferPool;
    bool usarLRU;
    int punteroReloj;
    std::list<int> listaLRU;
    std::unordered_map<int, std::list<int>::iterator> mapaLRU;

public:
    BufferManager(BufferPool& pool) : bufferPool(pool), punteroReloj(0) {
        std::srand(std::time(nullptr));
        usarLRU = std::rand() % 2;

        if (usarLRU) {
            std::cout << "Algoritmo seleccionado: LRU" << std::endl;
        } else {
            std::cout << "Algoritmo seleccionado: Clock" << std::endl;
        }
    }

    void MenuBufferManager() {
        while (true) {
            int pageID;
            std::cout << "Que pagina desea consultar? ";
            std::cin >> pageID;

            std::string nombreArchivo = "Paginas/" + std::to_string(pageID) + ".txt";
            std::ifstream archivoPagina(nombreArchivo);
            if (!archivoPagina.is_open()) {
                std::cerr << "No se pudo abrir el archivo de la página: " << nombreArchivo << std::endl;
                continue;
            }

            BufferPool::Frame* frame = BuscarFramePorPagina(pageID);
            if (!frame) {
                frame = BuscarFrameVacio();
                if (!frame) {
                    if (usarLRU) {
                        LRU(pageID);
                    } else {
                        AlgoritmoClock(pageID);
                    }
                    frame = BuscarFramePorPagina(pageID);
                } else {
                    frame->pageID = pageID;
                    frame->pinCount = 0;
                    frame->dirtyBit = false;
                }
            }

            frame->pinCount++;
            frame->pinned = true;
            if (usarLRU) {
                ActualizarLRU(pageID);
            } else {
                frame->refBit = true; // Actualizar bit de referencia
            }
            bufferPool.ActualizarTabla();

            char accion;
            while (true) {
                std::cout << "Desea escribir (e) o leer (l) la pagina? ";
                std::cin >> accion;
                if (accion == 'l') {
                    std::string line;
                    while (std::getline(archivoPagina, line)) {
                        std::cout << line << std::endl;
                    }
                    archivoPagina.clear();
                    archivoPagina.seekg(0, std::ios::beg);
                } else if (accion == 'e') {
                    frame->dirtyBit = true;
                    std::string contenido;
                    std::cout << "Escribe el contenido: ";
                    std::cin.ignore();
                    std::getline(std::cin, contenido);

                    std::cout << "Desea guardar el contenido? (si/no): ";
                    std::string respuesta;
                    std::cin >> respuesta;
                    if (respuesta == "si") {
                        std::ofstream outFile(nombreArchivo, std::ios::trunc);
                        if (outFile.is_open()) {
                            outFile << contenido;
                            outFile.close();
                        } else {
                            std::cerr << "No se pudo escribir en el archivo de la página." << std::endl;
                        }
                    }
                } else {
                    std::cerr << "Acción no válida." << std::endl;
                    continue;
                }

                std::cout << "Desea liberar la pagina? (si/no): ";
                std::string respuesta;
                std::cin >> respuesta;
                if (respuesta == "si") {
                    frame->pinned = false;
                    bufferPool.ActualizarTabla();
                    break;
                } else {
                    continue;
                }
            }
            bufferPool.MostrarTabla();
        }
    }

    void ActualizarLRU(int pageID) {
        if (mapaLRU.find(pageID) != mapaLRU.end()) {
            listaLRU.erase(mapaLRU[pageID]);
        }
        listaLRU.push_front(pageID);
        mapaLRU[pageID] = listaLRU.begin();
    }

    void LRU(int pageID) {
        while (!listaLRU.empty()) {
            int lruPageID = listaLRU.back();
            listaLRU.pop_back();
            mapaLRU.erase(lruPageID);

            BufferPool::Frame* frame = BuscarFramePorPagina(lruPageID);
            if (frame && !frame->pinned) {
                frame->pageID = pageID;
                frame->pinCount = 0;
                frame->dirtyBit = false;
                frame->pinned = true;
                frame->refBit = false;
                ActualizarLRU(pageID);
                bufferPool.ActualizarTabla();
                return;
            }
        }
        std::cerr << "No se pudo encontrar una página no anclada para reemplazar." << std::endl;
    }

    void AlgoritmoClock(int pageID) {
        while (true) {
            BufferPool::Frame& frame = bufferPool.frames[punteroReloj];
            if (frame.refBit == false && frame.pinned == false) {
                frame.pageID = pageID;
                frame.dirtyBit = false;
                frame.pinned = true;
                frame.pinCount = 1;
                frame.refBit = true;
                bufferPool.ActualizarTabla();
                punteroReloj = (punteroReloj + 1) % bufferPool.frames.size();
                return;
            } else {
                frame.refBit = false;
                punteroReloj = (punteroReloj + 1) % bufferPool.frames.size();
            }
        }
    }

    BufferPool::Frame* BuscarFramePorPagina(int pageID) {
        for (auto& frame : bufferPool.frames) {
            if (frame.pageID == pageID) {
                return &frame;
            }
        }
        return nullptr;
    }

    BufferPool::Frame* BuscarFrameVacio() {
        for (auto& frame : bufferPool.frames) {
            if (frame.pageID == 0) { // Verificar si el frame está vacío
                return &frame;
            }
        }
        return nullptr;
    }
};

int main() {
    BufferPool bufferPool;
    bufferPool.MenuBufferPool();
    bufferPool.MostrarTabla();
    BufferManager bufferManager(bufferPool);
    bufferManager.MenuBufferManager();
    return 0;
}
