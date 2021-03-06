/*
 * Software License Agreement (BSD License)
 *
 *  Point Cloud Library (PCL) - www.pointclouds.org
 *  Copyright (c) 2010-2011, Willow Garage, Inc.
 *
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: point_representation.h 2319 2011-08-30 20:40:06Z jspricke $
 *
 */
#ifndef PCL_POINT_REPRESENTATION_H_
#define PCL_POINT_REPRESENTATION_H_

#include "pcl/point_types.h"
#include "pcl/win32_macros.h"

namespace pcl
{
  /** \brief @b PointRepresentation provides a set of methods for converting a point structs/object into an
    * n-dimensional vector.
    * \note This is an abstract class.  Subclasses must set nr_dimensions_ to the appropriate value in the constructor 
    * and provide an implemention of the pure virtual copyToFloatArray method.
    * \author Michael Dixon
    */
  template <typename PointT> 
  class PointRepresentation
  {
    protected:
      /** \brief The number of dimensions in this point's vector (i.e. the "k" in "k-D") */
      int nr_dimensions_;
      /** \brief A vector containing the rescale factor to apply to each dimension. */
      std::vector<float> alpha_;
      
    public:
      /** \brief Empty constructor */
      PointRepresentation () : nr_dimensions_ (0), alpha_ (0) {}
      
      /** \brief Copy point data from input point to a float array. This method must be overriden in all subclasses. 
       *  \param[in] p The input point
       *  \param[out] out A pointer to a float array.
       */
      virtual void copyToFloatArray (const PointT &p, float *out) const = 0;
      
      /** \brief Verify that the input point is valid.
       *  \param p The point to validate
       */
      virtual bool 
      isValid (const PointT &p) const
      {
        float *temp = new float[nr_dimensions_];
        copyToFloatArray (p, temp);
        bool is_valid = true;
        for (int i = 0; i < nr_dimensions_; ++i)
        {
          if (!pcl_isfinite (temp[i]))
          {
            is_valid = false;
            break;
          }
        }
        delete [] temp;
        return (is_valid);
      }
      
      /** \brief Convert input point into a vector representation, rescaling by \a alpha.
        * \param[in] p the input point
        * \param[out] out The output vector.  Can be of any type that implements the [] operator.
        */
      template <typename OutputType> void
      vectorize (const PointT &p, OutputType &out) const
      {
        float *temp = new float[nr_dimensions_];
        copyToFloatArray (p, temp);
        if (alpha_.empty ())
        {
          for (int i = 0; i < nr_dimensions_; ++i)
            out[i] = temp[i];
        }
        else
        {
          for (int i = 0; i < nr_dimensions_; ++i)
            out[i] = temp[i] * alpha_[i];
        }
        delete [] temp;
      }
      
      /** \brief Set the rescale values to use when vectorizing points
        * \param[in] rescale_array The array/vector of rescale values.  Can be of any type that implements the [] operator.
        */
      void 
      setRescaleValues (const float *rescale_array)
      {
        alpha_.resize (nr_dimensions_);
        for (int i = 0; i < nr_dimensions_; ++i)
          alpha_[i] = rescale_array[i];
      }
      
      /** \brief Return the number of dimensions in the point's vector representation. */
      inline int getNumberOfDimensions () const { return (nr_dimensions_); }
  };

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /** \brief @b DefaultPointRepresentation extends PointRepresentation to define default behavior for common point types.
   */
  template <typename PointDefault>
  class DefaultPointRepresentation : public PointRepresentation <PointDefault>
  {
    using PointRepresentation <PointDefault>::nr_dimensions_;

    public:
      // Boost shared pointers
      typedef boost::shared_ptr<DefaultPointRepresentation<PointDefault> > Ptr;
      typedef boost::shared_ptr<const DefaultPointRepresentation<PointDefault> > ConstPtr;

      DefaultPointRepresentation ()
      {
        // If point type is unknown, assume it's a struct/array of floats, and compute the number of dimensions
        nr_dimensions_ = sizeof (PointDefault) / sizeof (float);
        // Limit the default representation to the first 3 elements
        if (nr_dimensions_ > 3) nr_dimensions_ = 3;
      }

      inline Ptr 
      makeShared () const 
      { 
        return (Ptr (new DefaultPointRepresentation<PointDefault> (*this)));
      }

      virtual void 
      copyToFloatArray (const PointDefault &p, float * out) const
      {
        // If point type is unknown, treat it as a struct/array of floats
        const float * ptr = (float *)&p;
        for (int i = 0; i < nr_dimensions_; ++i)
          out[i] = ptr[i];
      }
  };

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  template <>
  class DefaultPointRepresentation <PointXYZ> : public  PointRepresentation <PointXYZ>
  {
    public:
      DefaultPointRepresentation ()
      {
        nr_dimensions_ = 3;
      }

      virtual void 
      copyToFloatArray (const PointXYZ &p, float * out) const
      {
        out[0] = p.x;
        out[1] = p.y;
        out[2] = p.z;
      }
  };

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  template <>
  class DefaultPointRepresentation <PointXYZI> : public  PointRepresentation <PointXYZI>
  {
    public:
      DefaultPointRepresentation ()
      {
        nr_dimensions_ = 3;
      }

      virtual void 
      copyToFloatArray (const PointXYZI &p, float * out) const
      {
        out[0] = p.x;
        out[1] = p.y;
        out[2] = p.z;
        // By default, p.intensity is not part of the PointXYZI vectorization
      }
  };

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  template <>
  class DefaultPointRepresentation <PointNormal> : public  PointRepresentation <PointNormal>
  {
    public:
      DefaultPointRepresentation ()
      {
        nr_dimensions_ = 3;
      }

      virtual void 
      copyToFloatArray (const PointNormal &p, float * out) const
      {
        out[0] = p.x;
        out[1] = p.y;
        out[2] = p.z;
      }
  };

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  template <>
  class DefaultPointRepresentation <PFHSignature125> : public  PointRepresentation <PFHSignature125>
  {
    public:
      DefaultPointRepresentation ()
      {
        nr_dimensions_ = 125;
      }

      virtual void 
        copyToFloatArray (const PFHSignature125 &p, float * out) const
      {
        for (int i = 0; i < nr_dimensions_; ++i)
          out[i] = p.histogram[i];
      }
  };

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  template <>
  class DefaultPointRepresentation <FPFHSignature33> : public  PointRepresentation <FPFHSignature33>
  {
    public:
      DefaultPointRepresentation ()
      {
        nr_dimensions_ = 33;
      }

      virtual void 
      copyToFloatArray (const FPFHSignature33 &p, float * out) const
      {
        for (int i = 0; i < nr_dimensions_; ++i)
          out[i] = p.histogram[i];
      }
  };

  template <>
  class DefaultPointRepresentation <VFHSignature308> : public  PointRepresentation <VFHSignature308>
  {
    public:
      DefaultPointRepresentation ()
      {
        nr_dimensions_ = 308;
      }

      virtual void
        copyToFloatArray (const VFHSignature308 &p, float * out) const
      {
        for (int i = 0; i < nr_dimensions_; ++i)
          out[i] = p.histogram[i];
      }
  };

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  template <>
  class DefaultPointRepresentation <NormalBasedSignature12> : public PointRepresentation <NormalBasedSignature12>
  {
    public:
      DefaultPointRepresentation ()
      {
        nr_dimensions_ = 12;
      }

      virtual void
        copyToFloatArray (const NormalBasedSignature12 &p, float * out) const
      {
        for (int i = 0; i < nr_dimensions_; ++i)
          out[i] = p.values[i];
      }
  };


  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /** \brief @b CustomPointRepresentation extends PointRepresentation to allow for sub-part selection on the point.
   */
  template <typename PointDefault>
  class CustomPointRepresentation : public PointRepresentation <PointDefault>
  {
    using PointRepresentation <PointDefault>::nr_dimensions_;

    /** \brief Use at most this many dimensions (i.e. the "k" in "k-D" is at most max_dim_) -- \note float fields are assumed */
    int max_dim_;
    /** \brief Use dimensions only starting with this one (i.e. the "k" in "k-D" is = dim - start_dim_) -- \note float fields are assumed */
    int start_dim_;

    public:
      // Boost shared pointers
      typedef boost::shared_ptr<CustomPointRepresentation<PointDefault> > Ptr;
      typedef boost::shared_ptr<const CustomPointRepresentation<PointDefault> > ConstPtr;

      /** \brief Constructor
        * \param[in] max_dim the maximum number of dimensions to use
        * \param[in] start_dim the starting dimension
        */
      CustomPointRepresentation (int max_dim = 3, int start_dim = 0) 
        : max_dim_(max_dim), start_dim_(start_dim)
      {
        // If point type is unknown, assume it's a struct/array of floats, and compute the number of dimensions
        nr_dimensions_ = sizeof (PointDefault) / sizeof (float) - start_dim_;
        // Limit the default representation to the first 3 elements
        if (nr_dimensions_ > max_dim_) 
          nr_dimensions_ = max_dim_;
      }

      inline Ptr 
      makeShared () const 
      { 
        return Ptr (new CustomPointRepresentation<PointDefault> (*this)); 
      }

      /** \brief Copy the point data into a float array
        * \param[in] p the input point
        * \param[out] out the resultant output array
        */
      virtual void
      copyToFloatArray (const PointDefault &p, float *out) const
      {
        // If point type is unknown, treat it as a struct/array of floats
        const float *ptr = ((float*)&p) + start_dim_;
        for (int i = 0; i < nr_dimensions_; ++i)
          out[i] = ptr[i];
      }
  };
}

#endif // #ifndef PCL_POINT_REPRESENTATION_H_
