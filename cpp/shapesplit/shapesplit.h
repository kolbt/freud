// Copyright (c) 2010-2016 The Regents of the University of Michigan
// This file is part of the Freud project, released under the BSD 3-Clause License.

#include <tbb/tbb.h>
#include <ostream>

// work around nasty issue where python #defines isalpha, toupper, etc....
#undef __APPLE__
#include <Python.h>
#define __APPLE__

#include <memory>

#include "VectorMath.h"
#include "box.h"
#include "Index1D.h"

#ifndef _SHAPESPLIT_H__
#define _SHAPESPLIT_H__

/*! \file ShapeSplit.h
    \brief Routines for computing radial density functions
*/

namespace freud { namespace shapesplit {

//! Split a given set of points into more points off a set of local vectors
/*! A given set of points is given and split into Np*Nsplit points.
*/
class ShapeSplit
    {
    public:
        //! Constructor
        ShapeSplit();

        //! Update the simulation box
        void updateBox(box::Box& box);

        //! Get the simulation box
        const box::Box& getBox() const
            {
            return m_box;
            }

        //! Compute the RDF
        void compute(const vec3<float> *points,
                     unsigned int Np,
                     const quat<float> *orientations,
                     const vec3<float> *split_points,
                     unsigned int Nsplit);

        // //! Python wrapper for compute
        // void computePy(box::Box& box,
        //                boost::python::numeric::array points,
        //                boost::python::numeric::array orientations,
        //                boost::python::numeric::array split_points);

        //! Get a reference to the last computed split shape
        std::shared_ptr<float> getShapeSplit()
            {
            return m_split_array;
            }

        // //! Python wrapper for getShapeSplit() (returns a copy)
        // boost::python::numeric::array getShapeSplitPy()
        //     {
        //     float *arr = m_split_array.get();
        //     std::vector<intp> dims(3);
        //     dims[0] = m_Np;
        //     dims[1] = m_Nsplit;
        //     dims[2] = 3;
        //     return num_util::makeNum(arr, dims);
        //     }

        //! Get a reference to the last computed split orientations
        std::shared_ptr<float> getShapeOrientations()
            {
            return m_orientation_array;
            }

        // //! Python wrapper for getShapeOrientations() (returns a copy)
        // boost::python::numeric::array getShapeOrientationsPy()
        //     {
        //     float *arr = m_orientation_array.get();
        //     std::vector<intp> dims(3);
        //     dims[0] = m_Np;
        //     dims[1] = m_Nsplit;
        //     dims[2] = 4;
        //     return num_util::makeNum(arr, dims);
        //     }

    private:
        box::Box m_box;            //!< Simulation box the particles belong in
        unsigned int m_Np;
        unsigned int m_Nsplit;

        std::shared_ptr<float> m_split_array;
        std::shared_ptr<float> m_orientation_array;
    };

}; }; // end namespace freud::shapesplit

#endif // _SHAPESPLIT_H__
