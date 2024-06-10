#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <list>
#include <cstdlib>
#include <ctime>
// ---------- Autores: Lizardo Castillo y Karla Cornejo ---------- //

/*Autores: Lizardo Castillo y Karla Cornejo*/
// Clase que gestiona el conjunto de frames en el buffer pool
class BufferPool {
private:
    /*Autor: Lizardo Castillo*/
    // Representa un frame individual en el buffer pool
    struct Frame {
        int frameID;    // Identificador único del frame
        int pageID;     // Identificador de la página contenida en el frame
        bool dirtyBit;  // Indica si el contenido del frame ha sido modificado
        bool pinned;    // Indica si el frame está anclado (bloqueado)
        int pinCount;   // Número de veces que el frame ha sido anclado
        bool refBit;    // Bit de referencia para el algoritmo Clock
    };

    std::vector<Frame> frames; // Vector de frames en el buffer pool
    std::string tablaFilename; // Nombre del archivo que contiene la tabla del estado de los frames

public:
    /*Autora: Karla Cornejo*/
    // Constructor que inicializa el nombre del archivo de la tabla
    BufferPool() : tablaFilename("TablaBufferPool.txt") {}

    /*Autor: Lizardo Castillo*/
    // Permite al usuario especificar el número de frames y los inicializa con valores predeterminados
    void MenuBufferPool() {
        int numFrames;
        std::cout << "Ingrese la cantidad de frames que tendra el buffer: ";
        std::cin >> numFrames;
        frames.resize(numFrames);

        for (int i = 0; i < numFrames; ++i) {
            frames[i] = {i, 0, false, false, 0, false}; // Inicializar pageID con 0 para indicar frame vacío
        }

        CrearTabla();
    }

    /*Autor: Lizardo Castillo*/
    // Crea un archivo con la información del estado actual de los frames
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

    /*Autor: Lizardo Castillo*/
    // Muestra el contenido de la tabla del buffer pool en la consola
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

    /*Autora: Karla Cornejo*/
    // Actualiza la tabla con la información actualizada de los frames
    void ActualizarTabla() {
        CrearTabla();
    }

    /*Autora: Karla Cornejo*/
    // Permite que BufferManager acceda a los miembros privados de BufferPool
    friend class BufferManager;
};

/*Autores: Lizardo Castillo y Karla Cornejo*/
// Clase que administra el reemplazo de páginas en el buffer pool utilizando los algoritmos LRU o Clock
class BufferManager {
private:
    BufferPool& bufferPool;         // Referencia al buffer pool
    bool usarLRU;                   // Indica si se está usando el algoritmo LRU
    int punteroReloj;               // Puntero para el algoritmo Clock
    std::list<int> listaLRU;        // Lista para el algoritmo LRU
    std::unordered_map<int, std::list<int>::iterator> mapaLRU;  // Mapa para acceso rápido en LRU

public:
    /*Autora: Karla Cornejo*/
    // Constructor que selecciona aleatoriamente el algoritmo de reemplazo (LRU o Clock)
    BufferManager(BufferPool& pool) : bufferPool(pool), punteroReloj(0) {
        std::srand(std::time(nullptr));
        usarLRU = std::rand() % 2;

        if (usarLRU) {
            std::cout << "Algoritmo seleccionado: LRU" << std::endl;
        } else {
            std::cout << "Algoritmo seleccionado: Clock" << std::endl;
        }
    }

    /*Autora: Karla Cornejo*/
    // Interactúa con el usuario para gestionar el acceso y modificación de las páginas en el buffer pool
    void MenuBufferManager() {
        while (true) {
            int pageID;
            std::cout << "Que pagina desea consultar? ";
            std::cin >> pageID;

            std::string nombreArchivo = "Paginas/" + std::to_string(pageID) + ".txt";
            std::ifstream archivoPagina(nombreArchivo);
            if (!archivoPagina.is_open()) {
                std::cerr << "No se pudo abrir el archivo de la pagina: " << nombreArchivo << std::endl;
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
                            std::cerr << "No se pudo escribir en el archivo de la pagina." << std::endl;
                        }
                    }
                } else {
                    std::cerr << "Acción no valida." << std::endl;
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

    /*Autores: Lizardo Castillo y Karla Cornejo*/
    // Actualiza la lista LRU al acceder a una página, moviéndola al frente de la lista
    void ActualizarLRU(int pageID) {
        if (mapaLRU.find(pageID) != mapaLRU.end()) {
            listaLRU.erase(mapaLRU[pageID]);
        }
        listaLRU.push_front(pageID);
        mapaLRU[pageID] = listaLRU.begin();
    }

    /*Autores: Lizardo Castillo y Karla Cornejo*/
    // Implementa el algoritmo LRU para el reemplazo de páginas cuando el buffer pool está lleno
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

    /*Autores: Lizardo Castillo y Karla Cornejo*/
    // Implementa el algoritmo Clock para el reemplazo de páginas cuando el buffer pool está lleno
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

    /*Autora: Karla Cornejo*/
    // Busca y retorna un frame en el buffer pool que contiene la página especificada por pageID
    BufferPool::Frame* BuscarFramePorPagina(int pageID) {
        for (auto& frame : bufferPool.frames) {
            if (frame.pageID == pageID) {
                return &frame;
            }
        }
        return nullptr;
    }

    /*Autora: Karla Cornejo*/
    // Busca y retorna un frame vacío (sin página) en el buffer pool
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
