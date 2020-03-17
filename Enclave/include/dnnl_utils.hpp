#ifndef EXAMPLE_UTILS_HPP
#define EXAMPLE_UTILS_HPP

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <stdlib.h>
#include <string>
#include <initializer_list>

#include "dnnl.hpp"
#include "dnnl_debug.h"
#include "enclave.h"

// Exception class to indicate that the example uses a feature that is not
// available on the current systems. It is not treated as an error then, but
// just notifies a user.
struct example_allows_unimplemented : public std::exception {
    example_allows_unimplemented(const char *message) noexcept
        : message(message) {}
    virtual const char *what() const noexcept override { return message; }
    const char *message;
};

inline const char *engine_kind2str_upper(dnnl::engine::kind kind);

// Runs example function with signature void() and catches errors.
// Returns `0` on success, `1` or DNNL error, and `2` on example error.
inline int handle_errors(
        std::initializer_list<dnnl::engine::kind> engine_kinds,
        std::function<void()> example) {
    int exit_code = 0;

    try {
        example();
    } catch (example_allows_unimplemented &e) {
        printf(e.message);
        exit_code = 0;
    } catch (dnnl::error &e) {
        printf( "DNNL error caught: \n \tStatus %s\n \tMessage %s\n", 
                dnnl_status2str(e.status), e.what());
        exit_code = 1;
    } catch (std::exception &e) {
        printf( "Error in the example: %s" , e.what() );
        exit_code = 2;
    }

    std::string engine_kind_str;
    for (auto it = engine_kinds.begin(); it != engine_kinds.end(); ++it) {
        if (it != engine_kinds.begin()) engine_kind_str += "/";
        engine_kind_str += engine_kind2str_upper(*it);
    }
    return exit_code;
}

// Same as above, but for functions with signature
// void(dnnl::engine::kind engine_kind, int argc, char **argv).
inline int handle_errors(
        std::function<void(dnnl::engine::kind, int, char **)> example,
        dnnl::engine::kind engine_kind, int argc, char **argv) {
    return handle_errors(
            {engine_kind}, [&]() { example(engine_kind, argc, argv); });
}
inline int handle_errors(
        std::function<void(dnnl_transfer_layer_data*)> example,
        dnnl_transfer_layer_data* data) {
            auto engine_kind = dnnl::engine::kind::cpu;
    return handle_errors(
            {engine_kind}, [&]() { example(data); });
}
// Same as above, but for functions with signature void(dnnl::engine::kind).
inline int handle_errors(
        std::function<void(dnnl::engine::kind)> example,
        dnnl::engine::kind engine_kind) {
    return handle_errors(
            {engine_kind}, [&]() { example(engine_kind); });
}

inline dnnl::engine::kind parse_engine_kind(
        int argc, char **argv, int extra_args = 0) {
    // Returns default engine kind, i.e. CPU, if none given
    if (argc == 1) {
        return dnnl::engine::kind::cpu;
    } else if (argc <= extra_args + 2) {
        std::string engine_kind_str = argv[1];
        // Checking the engine type, i.e. CPU or GPU
        if (engine_kind_str == "cpu") {
            return dnnl::engine::kind::cpu;
        } else if (engine_kind_str == "gpu") {
            // Checking if a GPU exists on the machine
            if (dnnl::engine::get_count(dnnl::engine::kind::gpu) == 0) {
                printf( "Could not find compatible GPU" ,
                             "Please run the example with CPU instead"
                             );
                
            }
            return dnnl::engine::kind::gpu;
        }
    }

    // If all above fails, the example should be ran properly
    printf( "Inappropriate engine kind." 
                , "Please run the example like this: " , argv[0] , " [cpu|gpu]"
                , (extra_args ? " [extra arguments]" : "") , "."  );
}

inline const char *engine_kind2str_upper(dnnl::engine::kind kind) {
    if (kind == dnnl::engine::kind::cpu) return "CPU";
    if (kind == dnnl::engine::kind::gpu) return "GPU";
    assert(!"not expected");
    return "<Unknown engine>";
}

// Read from memory, write to handle
inline void read_from_dnnl_memory(void *handle, dnnl::memory &mem) {
    dnnl::engine eng = mem.get_engine();
    size_t bytes = mem.get_desc().get_size();

    if (eng.get_kind() == dnnl::engine::kind::cpu) {
        uint8_t *src = static_cast<uint8_t *>(mem.get_data_handle());
        for (size_t i = 0; i < bytes; ++i)
            ((uint8_t *)handle)[i] = src[i];
    }
#if DNNL_GPU_RUNTIME == DNNL_RUNTIME_OCL
    else if (eng.get_kind() == dnnl::engine::kind::gpu) {
        dnnl::stream s(eng);
        cl_command_queue q = s.get_ocl_command_queue();
        cl_mem m = mem.get_ocl_mem_object();

        cl_int ret = clEnqueueReadBuffer(
                q, m, CL_TRUE, 0, bytes, handle, 0, NULL, NULL);
        if (ret != CL_SUCCESS)
            throw std::runtime_error("clEnqueueReadBuffer failed.");
    }
#endif
}

// Read from handle, write to memory
inline void write_to_dnnl_memory(void *handle, dnnl::memory &mem) {
    dnnl::engine eng = mem.get_engine();
    size_t bytes = mem.get_desc().get_size();

    if (eng.get_kind() == dnnl::engine::kind::cpu) {
        uint8_t *dst = static_cast<uint8_t *>(mem.get_data_handle());
        for (size_t i = 0; i < bytes; ++i)
            dst[i] = ((uint8_t *)handle)[i];
    }
#if DNNL_GPU_RUNTIME == DNNL_RUNTIME_OCL
    else if (eng.get_kind() == dnnl::engine::kind::gpu) {
        dnnl::stream s(eng);
        cl_command_queue q = s.get_ocl_command_queue();
        cl_mem m = mem.get_ocl_mem_object();
        size_t bytes = mem.get_desc().get_size();

        cl_int ret = clEnqueueWriteBuffer(
                q, m, CL_TRUE, 0, bytes, handle, 0, NULL, NULL);
        if (ret != CL_SUCCESS)
            throw std::runtime_error("clEnqueueWriteBuffer failed.");
    }
#endif
}

#endif