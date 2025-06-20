#include "BufferManager.h"
#include "DiscoFisico.h"
#include <string.h>
#include "generales.h"



DiscoFisico * discoUsado;
//* ---------------------- Buffer Manager ----------------------
BufferManager::BufferManager(int num_frames, DiscoFisico * mydisk){
    this-> buffer_pool= new BufferPool(num_frames);
    discoUsado=mydisk;
}

BufferManager::~BufferManager(){
    delete buffer_pool;
}

//! falta usar operación
string * BufferManager::acceder(int id_bloque, Operacion op){
    ver_tabla();
    int idx = buffer_pool->buscar_pagina_id(id_bloque);
    if(idx != -1){
        // HIT: actualizar tiempo de uso
        buffer_pool->actualizar_tiempo_uso(idx);
        buffer_pool->incrementar_pin_count(idx);
        buffer_pool->incrementar_hit();
        buffer_pool->pin(idx);
        /* if(op == Operacion::Insertar){
            buffer_pool->high_dirty_bit(idx);
        } */
        ver_tabla();
        return buffer_pool->get_puntero(idx);
    } else{
        // MISS: cargar página
        int frame_libre = buffer_pool->cargar_pagina(id_bloque, op);
        if (frame_libre==-1)
            return NULL;
        buffer_pool->pin(frame_libre);
        buffer_pool->incrementar_miss();
        ver_tabla();
        return buffer_pool->get_puntero(frame_libre);
    }
}
void BufferManager::ver_tabla(){
    //    ,is_pin?"true":"false",pin_count,last_used);
    printf("|FrameId|PageId\t|Dirty Bit\t|is Pin\t|Pin Count\t|Last Used|\n");
    buffer_pool->print();
    buffer_pool->print_hit_rate();
}

void BufferManager::high_dirty_bit(int id){
    buffer_pool->high_dirty_bit(id);
}

void BufferManager::pin(int id){
    buffer_pool->pin(id);
}

void BufferManager::unpin(int id){
    buffer_pool->unpin(id);
}

void BufferManager::guardar(int id){
    int idx=buffer_pool->buscar_pagina_id(id);
    buffer_pool->guardar(idx);
}

void BufferManager::eliminar(int id){
    int idx=buffer_pool->buscar_pagina_id(id);
    buffer_pool->eliminar(idx);
}

//* ---------------------- Buffer Pool----------------------
BufferPool::BufferPool(int num_frames){
    listaBuffer = new Frame[num_frames];
    this->num_frames = num_frames;
    num_hit = 0;
    num_miss =0;
    tiempo_global = 0;
    for (int i = 0; i < num_frames; i++){
        listaBuffer[i] = Frame(i);
    }
}

BufferPool::~BufferPool(){
    delete[] listaBuffer;
}

int BufferPool::buscar_pagina_id(int id){
    for (int i = 0; i < num_frames; i++){
        if(listaBuffer+i!=NULL and id == listaBuffer[i].get_id())
            return i;
    }
    return -1;
}

void BufferPool::print_hit_rate(){
    printf("#hits = %d\n#miss = %d\nhit_rate = %.2f\n",num_hit,num_miss,(float)num_hit/(num_hit+num_miss));
}

void BufferPool::high_dirty_bit(int id){
    for (int i = 0; i < num_frames; i++){
        if (listaBuffer[i].get_id() == id){
            listaBuffer[i].high_dirty_bit();
            break;
        }
    }
    
    //listaBuffer[idx].high_dirty_bit();
}

int BufferPool::tarjet_eliminar(){
    int lru_idx = -1;
    int min_time;
    bool first_found = false;
    
    for (int i = 0; i < num_frames; i++){
        if(!listaBuffer[i].get_is_pin()){
            if(!first_found){
                min_time = listaBuffer[i].get_last_used();
                lru_idx = i;
                first_found = true;
            } else if(listaBuffer[i].get_last_used() < min_time){
                min_time = listaBuffer[i].get_last_used();
                lru_idx = i;
            }
        }
    }
    return lru_idx;
}

void BufferPool::eliminar(int idx){
    guardar(idx);
    listaBuffer[idx].reset_frame();
}

void BufferPool::guardar(int idx){
    if(listaBuffer[idx].get_dirty_bit()){
        int opcion;
        printf("desea guardar cambios? de %d\n si == 1\n no== 0\nopcion:    ",listaBuffer[idx].get_id());
        scanf("%d",&opcion);
        getchar();
        if (opcion){
            discoUsado->reemplazar(listaBuffer[idx].get_id(),listaBuffer[idx].get_puntero());
            printf("bloque %d actualizado\n",listaBuffer[idx].get_id());
        }
        listaBuffer[idx].low_dirty_bit();       
    }
}

int BufferPool::cargar_pagina(int id_bloque, Operacion op){
    // Buscar frame libre
    int frame_idx = buscar_frame_libre();
    
    if(frame_idx == -1){
        // No hay frames libres, aplicar LRU
        frame_idx = tarjet_eliminar();
        if(frame_idx != -1){
            eliminar(frame_idx);
        }
    }
    
    // Cargar nueva página
    if(frame_idx != -1){
        Page* nueva_pagina = new Page(id_bloque);

        
        if (!listaBuffer[frame_idx].set_pagina(nueva_pagina))
            return -1;
        listaBuffer[frame_idx].set_last_used(++tiempo_global);
        
        if(op == Operacion::Insertar){
            listaBuffer[frame_idx].high_dirty_bit();
        }
    }
    printf("pagina cargada\n");
    return frame_idx;
}

int BufferPool::buscar_frame_libre(){
    for (int i = 0; i < num_frames; i++){
        if(listaBuffer[i].get_id() == -1){
            return i;
        }
    }
    return -1;
}


string * BufferPool::get_puntero(int idx){
    return listaBuffer[idx].get_puntero();
}

void BufferPool::actualizar_tiempo_uso(int idx){
    listaBuffer[idx].set_last_used(++tiempo_global);
}

void BufferPool::incrementar_pin_count(int idx){
    listaBuffer[idx].incrementar_pin_count();
}

void BufferPool::incrementar_hit(){
    num_hit++;
}

void BufferPool::incrementar_miss(){
    num_miss++;
}

void BufferPool::print(){
    for (int i = 0; i < num_frames; i++){
            listaBuffer[i].ver_atributos();
    }
    
}

void BufferPool::pin(int id){
    listaBuffer[id].pin();
}

void BufferPool::unpin(int id){
    //buscar_pagina_id(id);
    listaBuffer[buscar_pagina_id(id)].unpin();
}


//* ---------------------- Frame ----------------------
Frame::Frame(){
}
Frame::Frame(int i){
    id=i;
    pagina = NULL;
    pin_count = 1;
    dirty_bit = false;
    is_pin= false;
    last_used = 0;
}
Frame::~Frame(){}
//destructor default
void Frame::ver_atributos(){
    //comenzamos despues del frame Id
    if(pagina==NULL){
        printf("|%d\t|-\t|---\t\t|---\t|\t-\t|\t-|\n",id);
        return;
    }
    printf("|%d\t|%d\t|%s\t\t|%s\t|\t%d\t|\t%d|\n", id,
    pagina->get_id(),dirty_bit?"true":"false",is_pin?"true":"false",
    pin_count,last_used);
}

int Frame::get_id(){
    if(pagina!=NULL)
        return pagina->get_id();
    else
        return -1;
}

void Frame::set_last_used(int time){
    last_used = time;
}

bool Frame::set_pagina(Page* p){
    if(!p->valido()){
        printf("[-] id no valido\n");
        return 0;
    }
    pagina = p;
    return 1;
}

void Frame::reset_frame(){
    if(pagina != NULL){
        delete pagina;
        pagina = NULL;
    }
    pin_count = 1;
    dirty_bit = false;
    is_pin = false;
    last_used = 0;
}

void Frame::incrementar_pin_count(){
    pin_count++;
}


bool Frame::get_is_pin(){
    return is_pin;
}

bool Frame::get_dirty_bit(){
    return dirty_bit;
}

int Frame::get_last_used(){
    return last_used;
}

void Frame::high_dirty_bit(){
    dirty_bit = true;
}

void Frame::low_dirty_bit(){
    dirty_bit = false;
}


string * Frame::get_puntero(){
    if(pagina != NULL){
        return &(pagina->contenido);
    }
    return NULL;
}

void Frame::pin(){
    //printf("llegamos a pin frame\n");
    is_pin= true;
}

void Frame::unpin(){
    //printf("llegamos a unpin frame\n");
    is_pin= false;
    //ver_atributos();
}

//* ---------------------- Page ----------------------
Page::Page(int id){
    this->page_id = id;
    contenido = "";
    for (int i = 0; i < discoUsado->tam_bloque; i++){
        char ruta[20];
        //printf("buffer %s",buffer);
        if (!discoUsado->encontrarSector(ruta,id,i)){
            this->is_valido=false;
            return;
        }
        this->is_valido=true;
        contenido += discoUsado->leer(quitarEspacios(ruta));
    }
    printf("pagina inicializada con contenido\n");
}

Page::~Page(){}

int Page::get_id(){
    return page_id;
}

bool Page::valido(){
    return is_valido;
}



//* ---------------------- Generales----------------------



