
#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform vec3 uPosition = vec3(0.0f); 
uniform vec3 uScale = vec3(1.0f);
uniform vec3 uRotation = vec3(0.0f);

out vec3 outColor;

void main()
{
    vec3 scaledPos = aPos * uScale;  
    
    
    vec3 c = cos(uRotation);        // Cosine of the X, Y, and Z rotation angles
    vec3 s = sin(uRotation);        // Sine of the X, Y, and Z rotation angles
    
    // right-hand rotation direction, need to caclulate in order X, then Y, then Z
    
    vec3 rotatedAroundX = vec3(     // Rotate around X: preserve X, calculate new Y and Z.
            scaledPos.x,
            c.x * scaledPos.y - s.x * scaledPos.z, 
            s.x * scaledPos.y + c.x * scaledPos.z);
    
    vec3 rotatedAroundY = vec3(     // Rotate around Y: preserve Y, calculate new X and Z.
            s.y * rotatedAroundX.z + c.y * rotatedAroundX.x,
            rotatedAroundX.y,
            c.y * rotatedAroundX.z - s.y * rotatedAroundX.x); 
    
    vec3 rotatedAroundZ = vec3(     // Rotate around Z: preserve Z, calculate new X and Y.
            c.z * rotatedAroundY.x - s.z * rotatedAroundY.y, 
            s.z * rotatedAroundY.x + c.z * rotatedAroundY.y,
            rotatedAroundY.z);
    
    
    vec3 finalRotatedPos = rotatedAroundZ; 
    

    vec3 finalPosition = vec3(finalRotatedPos) + uPosition;
    
    gl_Position = vec4(finalPosition, 1.0);
    outColor = aColor;
}



// NOTE: reminder on what a right handed rotation means:
//      There are related conventions for the arrangement of coordinate axes and for positive rotation. 
//      For the formulas are used with right-handed rotation, positive rotation follows the right-hand rule:
//              1. Point your right thumb along the positive rotation axis.
//              2. Curl your fingers.
//              3. Their curl indicates the positive rotation direction.
//      
//      Viewed from the positive end of that axis looking toward the origin, the rotation is counter-clockwise. 
//      A left-hand rotation rule reverses that direction. 
//      Coordinate systems themselves can also be right-handed or left-handed; that describes the arrangement of their axes.
//
//          Positive 90° rotation
//      X rotation:	Positive Y → positive Z
//      Y rotation:	Positive Z → positive X
//      Z rotation:	Positive X → positive Y

