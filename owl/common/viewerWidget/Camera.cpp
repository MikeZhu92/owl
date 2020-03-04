// ======================================================================== //
// Copyright 2018-2019 Ingo Wald                                            //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "Camera.h"
#include "ViewerWidget.h"

namespace owl {
  namespace viewer {

    float computeStableEpsilon(float f)
    {
      return abs(f) * float(1./(1<<21));
    }

    float computeStableEpsilon(const vec3f v)
    {
      return max(max(computeStableEpsilon(v.x),
                     computeStableEpsilon(v.y)),
                 computeStableEpsilon(v.z));
    }

    void FullCamera::digestInto(SimpleCamera &easy)
    {
      easy.lens.center = position;
      easy.lens.radius = 0.f;
      easy.lens.du     = frame.vx;
      easy.lens.dv     = frame.vy;

      const float minFocalDistance
        = 1e1f*max(computeStableEpsilon(position),
                   computeStableEpsilon(frame.vx));

      /*
        tan(fov/2) = (height/2) / dist
        -> height = 2*tan(fov/2)*dist
      */
      float screen_height
        = 2.f*tanf(fovyInDegrees/2 * (float)M_PI/180.f)
        * max(minFocalDistance,focalDistance);
      easy.screen.vertical   = screen_height * frame.vy;
      easy.screen.horizontal = screen_height * aspect * frame.vx;
      easy.screen.lower_left
        = //easy.lens.center
        /* NEGATIVE z axis! */
        - max(minFocalDistance,focalDistance) * frame.vz
        - 0.5f * easy.screen.vertical
        - 0.5f * easy.screen.horizontal;

      easy.lastModified = getCurrentTime();
    }

    void FullCamera::setFovy(const float fovy)
    {
      this->fovyInDegrees = fovy;
    }

    void FullCamera::setAspect(const float aspect)
    {
      this->aspect = aspect;
    }

    void FullCamera::setFocalDistance(float focalDistance)
    {
      this->focalDistance = focalDistance;
    }

    /*! tilt the frame around the z axis such that the y axis is "facing upwards" */
    void FullCamera::forceUpFrame()
    {
      // frame.vz remains unchanged
      if (fabsf(dot(frame.vz,upVector)) < 1e-6f)
        // looking along upvector; not much we can do here ...
        return;
      frame.vx = normalize(cross(upVector,frame.vz));
      frame.vy = normalize(cross(frame.vz,frame.vx));
    }

    void FullCamera::setOrientation(/* camera origin    : */const vec3f &origin,
                                    /* point of interest: */const vec3f &interest,
                                    /* up-vector        : */const vec3f &up,
                                    /* fovy, in degrees : */float fovyInDegrees,
                                    /* set focal dist?  : */bool  setFocalDistance)
    {
      this->fovyInDegrees = fovyInDegrees;
      position = origin;
      upVector = up;
      frame.vz
        = (interest==origin)
        ? vec3f(0,0,1)
        : /* negative because we use NEGATIZE z axis */ - normalize(interest - origin);
      frame.vx = cross(up,frame.vz);
      if (dot(frame.vx,frame.vx) < 1e-8f)
        frame.vx = vec3f(0,1,0);
      else
        frame.vx = normalize(frame.vx);
      // frame.vx
      //   = (fabs(dot(up,frame.vz)) < 1e-6f)
      //   ? vec3f(0,1,0)
      //   : normalize(cross(up,frame.vz));
      frame.vy = normalize(cross(frame.vz,frame.vx));
      poiDistance = length(interest-origin);
      if (setFocalDistance) focalDistance = poiDistance;
      forceUpFrame();
    }


    /*! this gets called when the user presses a key on the keyboard ... */
    void FullCameraManip::key(char key, const vec2i &where)
    {
      FullCamera &fc = widget->fullCamera;

      switch(key) {
      case 'f':
      case 'F':
        if (widget->flyModeManip) widget->cameraManip = widget->flyModeManip;
        break;
      case 'i':
      case 'I':
        if (widget->inspectModeManip) widget->cameraManip = widget->inspectModeManip;
        break;
      case '+':
      case '=':
        fc.motionSpeed *= 2.f;
        std::cout << "# viewer: new motion speed is " << fc.motionSpeed << std::endl;
        break;
      case '-':
      case '_':
        fc.motionSpeed /= 2.f;
        std::cout << "# viewer: new motion speed is " << fc.motionSpeed << std::endl;
        break;
      case 'C':
        std::cout << "(C)urrent camera:" << std::endl;
        std::cout << "- from :" << fc.position << std::endl;
        std::cout << "- poi  :" << fc.getPOI() << std::endl;
        std::cout << "- upVec:" << fc.upVector << std::endl;
        std::cout << "- frame:" << fc.frame << std::endl;
        break;
      case 'x':
      case 'X':
        fc.setUpVector(fc.upVector==vec3f(1,0,0)?vec3f(-1,0,0):vec3f(1,0,0));
        widget->updateCamera();
        break;
      case 'y':
      case 'Y':
        fc.setUpVector(fc.upVector==vec3f(0,1,0)?vec3f(0,-1,0):vec3f(0,1,0));
        widget->updateCamera();
        break;
      case 'z':
      case 'Z':
        fc.setUpVector(fc.upVector==vec3f(0,0,1)?vec3f(0,0,-1):vec3f(0,0,1));
        widget->updateCamera();
        break;
      default:
        break;
      }
    }

  } // ::owl::viewer
} // ::owl

