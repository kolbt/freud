// Copyright (c) 2010-2016 The Regents of the University of Michigan
// This file is part of the Freud project, released under the BSD 3-Clause License.

#include <algorithm>
#include <stdexcept>
#include <complex>
#include <utility>
#include <vector>
#include <tbb/tbb.h>

#include "LocalDescriptors.h"
#include "ScopedGILRelease.h"
#include "HOOMDMatrix.h"

using namespace std;
using namespace tbb;
using hoomd::matrix::diagonalize;

/*! \file LocalDescriptors.h
  \brief Compute the hexatic order parameter for each particle
*/

namespace freud { namespace order {

LocalDescriptors::LocalDescriptors(
        unsigned int neighmax, unsigned int lmax, float rmax, bool negative_m):
    m_neighmax(neighmax), m_lmax(lmax),
    m_negative_m(negative_m), m_nn(rmax, neighmax), m_Nref(0), m_nNeigh(0)
    {
    }


void LocalDescriptors::computeNList(const box::Box& box, const vec3<float> *r_ref, unsigned int Nref, const vec3<float> *r, unsigned int Np)
    {
    m_nn.compute(box, r_ref, Nref, r, Np);
    }

void LocalDescriptors::compute(const box::Box& box, unsigned int nNeigh,
                               const vec3<float> *r_ref, unsigned int Nref,
                               const vec3<float> *r, unsigned int Np,
                               const quat<float> *q_ref,
                               LocalDescriptorOrientation orientation)
    {
    if(m_nn.getNref() != Nref || m_nn.getNp() != Np)
        throw runtime_error("Must call computeNList() before compute");

    // reallocate the output array if it is not the right size
    if (Nref != m_Nref || nNeigh != m_nNeigh)
        {
        m_sphArray = std::shared_ptr<complex<float> >(new complex<float>[nNeigh*Nref*getSphWidth()], std::default_delete<complex<float>[]>());
        m_nNeigh = nNeigh;
        }

    parallel_for(blocked_range<size_t>(0,Nref),
        [=] (const blocked_range<size_t>& br)
        {
        fsph::PointSPHEvaluator<float> sph_eval(m_lmax);
        Index2D idx_nlist(m_neighmax, 0);

        for(size_t i=br.begin(); i!=br.end(); ++i)
            {
            const vec3<float> r_i(r_ref[i]);

            vec3<float> rotation_0, rotation_1, rotation_2;

            if(orientation == LocalNeighborhood)
                {
                float inertiaTensor[3][3];
                for(size_t ii(0); ii < 3; ++ii)
                    for(size_t jj(0); jj < 3; ++jj)
                        inertiaTensor[ii][jj] = 0;

                for(size_t k(0); k < nNeigh; ++k)
                    {
                    const float rsq(m_nn.getRsqList().get()[idx_nlist(k, i)]);
                    const vec3<float> r_j(r[m_nn.getNeighborList().get()[idx_nlist(k, i)]]);
                    const vec3<float> rvec(box.wrap(r_j - r_i));

                    for(size_t ii(0); ii < 3; ++ii)
                        inertiaTensor[ii][ii] += rsq;

                    inertiaTensor[0][0] -= rvec.x*rvec.x;
                    inertiaTensor[0][1] -= rvec.x*rvec.y;
                    inertiaTensor[0][2] -= rvec.x*rvec.z;
                    inertiaTensor[1][0] -= rvec.x*rvec.y;
                    inertiaTensor[1][1] -= rvec.y*rvec.y;
                    inertiaTensor[1][2] -= rvec.y*rvec.z;
                    inertiaTensor[2][0] -= rvec.x*rvec.z;
                    inertiaTensor[2][1] -= rvec.y*rvec.z;
                    inertiaTensor[2][2] -= rvec.z*rvec.z;
                    }

                float eigenvalues[3];
                float eigenvectors[3][3];

                diagonalize(inertiaTensor, eigenvalues, eigenvectors);

                // Sort eigenvalues and eigenvectors so that
                // eigenvalues is in ascending order. This is
                // a kind of gross way to do it, but it gets
                // the job done.
                if(eigenvalues[0] > eigenvalues[1])
                    {
                    std::swap(eigenvalues[0], eigenvalues[1]);
                    for(size_t ii(0); ii < 3; ++ii)
                        std::swap(eigenvectors[ii][0], eigenvectors[ii][1]);
                    }
                if(eigenvalues[1] > eigenvalues[2])
                    {
                    std::swap(eigenvalues[1], eigenvalues[2]);
                    for(size_t ii(0); ii < 3; ++ii)
                        std::swap(eigenvectors[ii][1], eigenvectors[ii][2]);
                    }
                if(eigenvalues[0] > eigenvalues[1])
                    {
                    std::swap(eigenvalues[0], eigenvalues[1]);
                    for(size_t ii(0); ii < 3; ++ii)
                        std::swap(eigenvectors[ii][0], eigenvectors[ii][1]);
                    }

                rotation_0 = vec3<float>(eigenvectors[0][0], eigenvectors[1][0], eigenvectors[2][0]);
                rotation_1 = vec3<float>(eigenvectors[0][1], eigenvectors[1][1], eigenvectors[2][1]);
                rotation_2 = vec3<float>(eigenvectors[0][2], eigenvectors[1][2], eigenvectors[2][2]);
                }
            else if(orientation == ParticleLocal)
                {
                const rotmat3<float> rotmat(conj(q_ref[i]));
                rotation_0 = rotmat.row0;
                rotation_1 = rotmat.row1;
                rotation_2 = rotmat.row2;
                }
            else if (orientation == Global)
                {
                rotation_0 = vec3<float>(1, 0, 0);
                rotation_1 = vec3<float>(0, 1, 0);
                rotation_2 = vec3<float>(0, 0, 1);
                }
            else
                {
                throw std::runtime_error("Uncaught orientation mode in LocalDescriptors::compute");
                }

            unsigned int sphCount(i*nNeigh*getSphWidth());

            for(size_t k(0); k < nNeigh; ++k)
                {
                const float rsq(m_nn.getRsqList().get()[idx_nlist(k, i)]);
                const vec3<float> r_j(r[m_nn.getNeighborList().get()[idx_nlist(k, i)]]);
                const vec3<float> rij(box.wrap(r_j - r_i));
                const vec3<float> bond(dot(rotation_0, rij),
                                       dot(rotation_1, rij),
                                       dot(rotation_2, rij));

                const float magR(sqrt(rsq));
                float theta(atan2(bond.y, bond.x)); // theta in [-pi..pi] initially
                if(theta < 0)
                    theta += 2*M_PI; // move theta into [0..2*pi]
                const float phi(acos(bond.z/magR)); // phi in [0..pi]

                sph_eval.compute(phi, theta);

                std::copy(sph_eval.begin(m_negative_m), sph_eval.end(), &m_sphArray.get()[sphCount]);
                sphCount += getSphWidth();
                }
            }
        });

    // save the last computed number of particles
    m_Nref = Nref;
    }

}; }; // end namespace freud::order
