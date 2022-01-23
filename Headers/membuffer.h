/* -*- C++ -*-
 * File:        membuffer.h
 * Created:     Thu Mar 10 2005
 * Description: temporary buffer implementation.
 *
 * $Id$
 */

#ifndef MEMBUFFER_H
#define MEMBUFFER_H

template <class T>
struct temp_buffer
{
    T*  pool;
    size_t size;
    
    temp_buffer (size_t sz) : pool(new T[sz]), size(sz) {}
    ~temp_buffer () { delete[] pool; } 
    
    operator T*() { return (pool); }
    void resize (size_t sz)
        { delete[] pool; pool = new T[sz]; size = sz; }
        
    void realloc (size_t sz)
    {
        T* new_pool = new T[sz];
        copy (pool, pool + min (size, sz), new_pool);
        
        delete[] pool;
        pool = new_pool;
        size = sz;
    }
};

#endif /* MEMBUFFER_H */
