// Copyright (c) 2010-2016 The Regents of the University of Michigan
// This file is part of the Freud project, released under the BSD 3-Clause License.

#include <memory>

#include "HOOMDMath.h"
#include "VectorMath.h"

#include "LinkCell.h"
#include "box.h"

#include <tbb/tbb.h>

#ifndef _CORRELATIONFUNCTION_H__
#define _CORRELATIONFUNCTION_H__

/*! \file CorrelationFunction.cc
    \brief Generic pairwise correlation functions
*/

namespace freud { namespace density {

//! Computes the pairwise correlation function <p*q>(r) between two sets of points with associated values p and q.
/*! Two sets of points and two sets of values associated with those
    points are given. Computing the correlation function results in an
    array of the expected (average) product of all values at a given
    radial distance.

    The values of r to compute the correlation function at are
    controlled by the rmax and dr parameters to the constructor. rmax
    determins the maximum r at which to compute the correlation
    function and dr is the step size for each bin.

    <b>2D:</b><br>
    CorrelationFunction properly handles 2D boxes. As with everything
    else in freud, 2D points must be passed in as 3 component vectors
    x,y,0. Failing to set 0 in the third component will lead to
    undefined behavior.

    <b>Self-correlation:</b><br>
    It is often the case that we wish to compute the correlation
    function of a set of points with itself. If given the same arrays
    for both points and ref_points, we omit accumulating the
    self-correlation value in the first bin.

*/
template<typename T>
class CorrelationFunction
    {
    public:
        //! Constructor
        CorrelationFunction(float rmax, float dr);

        //! Destructor
        ~CorrelationFunction();

        //! Get the simulation box
        const box::Box& getBox() const
            {
            return m_box;
            }

        //! Reset the PCF array to all zeros
        void resetCorrelationFunction();

        //! Python wrapper for reset method
        void resetCorrelationFunctionPy()
            {
            resetCorrelationFunction();
            }

        //! accumulate the correlation function
        void accumulate(const box::Box &box,
                        const vec3<float> *ref_points,
                        const T *ref_values,
                        unsigned int n_ref,
                        const vec3<float> *points,
                        const T *point_values,
                        unsigned int Np);

        //! \internal
        //! helper function to reduce the thread specific arrays into the boost array
        void reduceCorrelationFunction();

        //! Get a reference to the last computed rdf
        std::shared_ptr<T> getRDF();

        //! Get a reference to the bin counts array
        std::shared_ptr<unsigned int> getCounts()
            {
            return m_bin_counts;
            }

        //! Get a reference to the r array
        std::shared_ptr<float> getR()
            {
            return m_r_array;
            }

        unsigned int getNBins() const
            {
            return m_nbins;
            }

    private:
        box::Box m_box;            //!< Simulation box the particles belong in
        float m_rmax;                     //!< Maximum r at which to compute g(r)
        float m_dr;                       //!< Step size for r in the computation
        locality::LinkCell* m_lc;          //!< LinkCell to bin particles for the computation
        unsigned int m_nbins;             //!< Number of r bins to compute g(r) over
        unsigned int m_n_ref;                  //!< number of reference particles
        unsigned int m_Np;                  //!< number of check particles
        unsigned int m_frame_counter;       //!< number of frames calc'd

        std::shared_ptr<T> m_rdf_array;         //!< rdf array computed
        std::shared_ptr<unsigned int> m_bin_counts; //!< bin counts that go into computing the rdf array
        std::shared_ptr<float> m_r_array;           //!< array of r values that the rdf is computed at
        tbb::enumerable_thread_specific<unsigned int *> m_local_bin_counts;
        tbb::enumerable_thread_specific<T *> m_local_rdf_array;
    };

template<typename T>
class CombineOCF
    {
    private:
        unsigned int m_nbins;
        unsigned int *m_bin_counts;
        tbb::enumerable_thread_specific<unsigned int *>& m_local_bin_counts;
        T *m_rdf_array;
        tbb::enumerable_thread_specific<T *>& m_local_rdf_array;
        float m_n_ref;
    public:
        CombineOCF(unsigned int nbins,
                   unsigned int *bin_counts,
                   tbb::enumerable_thread_specific<unsigned int *>& local_bin_counts,
                   T *rdf_array,
                   tbb::enumerable_thread_specific<T *>& local_rdf_array,
                   float n_ref)
            : m_nbins(nbins), m_bin_counts(bin_counts), m_local_bin_counts(local_bin_counts), m_rdf_array(rdf_array),
              m_local_rdf_array(local_rdf_array), m_n_ref(n_ref)
        {
        }
        void operator()( const tbb::blocked_range<size_t> &myBin ) const;
    };

template<typename T>
class ComputeOCF
    {
    private:
        const unsigned int m_nbins;
        tbb::enumerable_thread_specific<unsigned int *>& m_bin_counts;
        tbb::enumerable_thread_specific<T *>& m_rdf_array;
        const box::Box m_box;
        const float m_rmax;
        const float m_dr;
        const locality::LinkCell *m_lc;
        const vec3<float> *m_ref_points;
        const T *m_ref_values;
        const unsigned int m_n_ref;
        const vec3<float> *m_points;
        const T *m_point_values;
        unsigned int m_Np;
    public:
        ComputeOCF(const unsigned int nbins,
                   tbb::enumerable_thread_specific<unsigned int *>& bin_counts,
                   tbb::enumerable_thread_specific<T *>& rdf_array,
                   const box::Box &box,
                   const float rmax,
                   const float dr,
                   const locality::LinkCell *lc,
                   const vec3<float> *ref_points,
                   const T *ref_values,
                   unsigned int n_ref,
                   const vec3<float> *points,
                   const T *point_values,
                   unsigned int Np)
            : m_nbins(nbins), m_bin_counts(bin_counts), m_rdf_array(rdf_array), m_box(box), m_rmax(rmax), m_dr(dr),
              m_lc(lc), m_ref_points(ref_points), m_ref_values(ref_values), m_n_ref(n_ref), m_points(points),
              m_point_values(point_values), m_Np(Np)
        {
        }
        void operator()( const tbb::blocked_range<size_t> &myR ) const;
    };



// /*! \internal
//     \brief Template function to check the type of a given correlation
//         function value array. Should be specialized for its argument.
// */
// template<typename T>
// void checkCFType(boost::python::numeric::array values);

// #include "CorrelationFunction.cc"

}; }; // end namespace freud::density

#endif // _CORRELATIONFUNCTION_H__
