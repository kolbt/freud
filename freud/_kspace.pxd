# Copyright (c) 2010-2016 The Regents of the University of Michigan
# This file is part of the Freud project, released under the BSD 3-Clause License.

from freud.util._Boost cimport shared_array
from freud.util._VectorMath cimport vec3, quat
from libcpp.complex cimport complex

cdef extern from "kspace.h" namespace "freud::kspace":
    cdef cppclass FTdelta:
        FTDelta()
        void set_K(vec3[float]*, unsigned int)
        void set_rq(unsigned int, vec3[float]*, quat[float]*)
        void set_density(float complex)
        void compute() nogil except +
        shared_array[float complex] getFT()

    cdef cppclass FTsphere:
        FTsphere()
        void set_K(vec3[float]*, unsigned int)
        void set_rq(unsigned int, vec3[float]*, quat[float]*)
        void set_density(float complex)
        void compute() nogil except +
        shared_array[float complex] getFT()
        void set_radius(const float)

    cdef cppclass FTpolyhedron:
        FTpolyhedron()
        void set_K(vec3[float]*, unsigned int)
        void set_rq(unsigned int, vec3[float]*, quat[float]*)
        void set_density(float complex)
        void set_params(unsigned int, vec3[float]*, unsigned int, unsigned int *, unsigned int *, \
            vec3[float]* norm, float *d, float *area, float volume)
        void compute() nogil except +
        shared_array[float complex] getFT()
