#include "intersection.h"

bool LineSegmentIntersection(const glm::vec3 &_line1Start, const glm::vec3 &_line1End,
                             const glm::vec3 &_line2Start, const glm::vec3 &_line2End,
                             glm::vec3 &_intersection ){
    glm::vec3 diffLA, diffLB;
    double compareA, compareB;
    diffLA = _line1End - _line1Start;
    diffLB = _line2End - _line2Start;
    compareA = diffLA.x*_line1Start.y - diffLA.y*_line1Start.x;
    compareB = diffLB.x*_line2Start.y - diffLB.y*_line2Start.x;
    if (
        (
         ( ( diffLA.x*_line2Start.y - diffLA.y*_line2Start.x ) < compareA ) ^
         ( ( diffLA.x*_line2End.y - diffLA.y*_line2End.x ) < compareA )
         )
        &&
        (
         ( ( diffLB.x*_line1Start.y - diffLB.y*_line1Start.x ) < compareB ) ^
         ( ( diffLB.x*_line1End.y - diffLB.y*_line1End.x) < compareB )
         )
        )
    {
        double lDetDivInv = 1 / ((diffLA.x*diffLB.y) - (diffLA.y*diffLB.x));
        _intersection.x =  -((diffLA.x*compareB) - (compareA*diffLB.x)) * lDetDivInv ;
        _intersection.y =  -((diffLA.y*compareB) - (compareA*diffLB.y)) * lDetDivInv ;
        
        return true;
    }
    
    return false;
};
