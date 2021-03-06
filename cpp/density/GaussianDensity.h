// Copyright (c) 2010-2016 The Regents of the University of Michigan
// This file is part of the Freud project, released under the BSD 3-Clause License.

#include <tbb/tbb.h>

// work around nasty issue where python #defines isalpha, toupper, etc....
#undef __APPLE__
#include <Python.h>
#define __APPLE__

#include <memory>

#include "HOOMDMath.h"
#include "VectorMath.h"

#include "box.h"
#include "Index1D.h"

#ifndef _GaussianDensity_H__
#define _GaussianDensity_H__

/*! \file GaussianDensity.h
    \brief Routines for computing Gaussian smeared densities from points
*/

namespace freud { namespace density {

//! Computes the the density of a system on a grid.
/*! Replaces particle positions with a gaussian and calculates the
        contribution from the grid based upon the the distance of the grid cell
        from the center of the Gaussian.
*/
class GaussianDensity
    {
    public:
        //! Constructor
        GaussianDensity(unsigned int width,
                        float r_cut,
                        float sigma);
        GaussianDensity(unsigned int width_x,
                        unsigned int width_y,
                        unsigned int width_z,
                        float r_cut,
                        float sigma);

        // Destructor
        ~GaussianDensity();

        //! Get the simulation box
        const box::Box& getBox() const
                {
                return m_box;
                }

        //! Reset the gaussian array to all zeros
        void resetDensity();

        //! \internal
        //! helper function to reduce the thread specific arrays into the boost array
        void reduceDensity();

        //! Compute the Density
        void compute(const box::Box& box, const vec3<float> *points, unsigned int Np);

        //!Get a reference to the last computed Density
        std::shared_ptr<float> getDensity();

        unsigned int getWidthX();

        unsigned int getWidthY();

        unsigned int getWidthZ();

    private:
        box::Box m_box;    //!< Simulation box the particles belong in
        unsigned int m_width_x,m_width_y,m_width_z;           //!< Num of bins on one side of the cube
        float m_rcut;                  //!< Max r at which to compute density
        float m_sigma;                  //!< Variance
        Index3D m_bi;                   //!< Bin indexer
        unsigned int m_frame_counter;       //!< number of frames calc'd

        std::shared_ptr<float> m_Density_array;            //! computed density array
        tbb::enumerable_thread_specific<float *> m_local_bin_counts;
    };

}; }; // end namespace freud::density

#endif
