#ifndef __UTILITY_HLSLI__
#define __UTILITY_HLSLI__

#define quaternion float4

float4 GetTranslation(in matrix mat)
{
    return float4(mat._41, mat._42, mat._43, mat._44);
}

float3 GetScale(in matrix mat)
{
  return float3
    (
     length(float3(mat[0][0], mat[0][1], mat[0][2])),
     length(float3(mat[1][0], mat[1][1], mat[1][2])),
     length(float3(mat[2][0], mat[2][1], mat[2][2]))
    );
}

quaternion MatToQuaternion(in matrix world)
{
  // http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/
  const float trace = world._11 + world._22 + world._33;
  quaternion  ret;

  // square root of w is not negative.
  if (trace > 0)
  {
    float s = 0.5f / sqrt(trace + 1.0); // S=4*qw 
    ret.w   = 0.25f / s;
    ret.x   = (world[2][1] - world[1][2]) * s;
    ret.y   = (world[0][2] - world[2][0]) * s;
    ret.z   = (world[1][0] - world[0][1]) * s;
  }
  else if ((world[0][0] > world[1][1]) && (world[0][0] > world[2][2]))
  {
    // x is not negative and greatest.
    float s = sqrt(1.0 + world[0][0] - world[1][1] - world[2][2]) * 2.f; // S=4*qx 
    ret.w   = (world[2][1] - world[1][2]) / s;
    ret.x   = 0.25f * s;
    ret.y   = (world[0][1] + world[1][0]) / s;
    ret.z   = (world[0][2] + world[2][0]) / s;
  }
  else if (world[1][1] > world[2][2])
  {
    // y is not negative and greatest.
    float s = sqrt(1.0 + world[1][1] - world[0][0] - world[2][2]) * 2.f; // S=4*qy
    ret.w   = (world[0][2] - world[2][0]) / s;
    ret.x   = (world[0][1] + world[1][0]) / s;
    ret.y   = 0.25f * s;
    ret.z   = (world[1][2] + world[2][1]) / s;
  }
  else
  {
    // z is not negative and greatest.
    float s = sqrt(1.0 + world._33 - world._11 - world._22) * 2.f; // S=4*qz
    ret.w   = (world[1][0] - world[0][1]) / s;
    ret.x   = (world[0][2] + world[2][0]) / s;
    ret.y   = (world[1][2] + world[2][1]) / s;
    ret.z   = 0.25f * s;
  }

  return ret;
}

matrix QuaternionToMat(in quaternion qt)
{
  // http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToMatrix/
  matrix ret = matrix
    (
     float4(0, 0, 0, 0),
     float4(0, 0, 0, 0),
     float4(0, 0, 0, 0),
     float4(0, 0, 0, 0)
    );

  float x  = qt.x,   y  = qt.y,   z  = qt.z, w = qt.w;
  float x2 = x + x,  y2 = y + y,  z2 = z + z;

  float xx = x * x2, xy = x * y2, xz = x * z2;
  float yy = y * y2, yz = y * z2, zz = z * z2;
  float wx = w * x2, wy = w * y2, wz = w * z2;

  ret[0][0] = 1.0 - (yy + zz);
  ret[0][1] = xy - wz;
  ret[0][2] = xz + wy;

  ret[1][0] = xy + wz;
  ret[1][1] = 1.0 - (xx + zz);
  ret[1][2] = yz - wx;

  ret[2][0] = xz - wy;
  ret[2][1] = yz + wx;
  ret[2][2] = 1.0 - (xx + yy);

  ret[3][3] = 1.0;

  return ret;
}

quaternion SLerp(quaternion v0, quaternion v1, float t)
{
  // Compute the cosine of the angle between the two vectors.
  float d = dot(v0, v1);

  const float DOT_THRESHOLD = 0.9995;
  if (abs(d) > DOT_THRESHOLD)
  {
    // If the inputs are too close for comfort, linearly interpolate
    // and normalize the result.
    return normalize(lerp(v0, v1, t));
  }

  // If the dot product is negative, the quaternions
  // have opposite handed-ness and slerp won't take
  // the shorter path. Fix by reversing one quaternion.
  if (d < 0.0f)
  {
    v1 = -v1;
    d  = -d;
  }

  clamp(d, -1, 1); // Robustness: Stay within domain of acos()
  float theta_0 = acos(d); // theta_0 = angle between input vectors
  float theta   = theta_0 * t; // theta = angle between v0 and result 

  float4 v2 = v1 - v0 * d;
  normalize(v2); // { v0, v2 } is now an orthonormal basis

  return v0 * cos(theta) + v2 * sin(theta);
}

matrix Compose(in float3 translation, in float3 scale, in float4 rotation)
{
  matrix ret = QuaternionToMat(rotation);

  // set the translation value.
  ret._41 = translation.x;
  ret._42 = translation.y;
  ret._43 = translation.z;
  ret._44 = 1.0f;

  // multiply matrix with scale for each column
  ret._11 *= scale.x;
  ret._21 *= scale.x;
  ret._31 *= scale.x;

  ret._12 *= scale.y;
  ret._22 *= scale.y;
  ret._32 *= scale.y;

  ret._13 *= scale.z;
  ret._23 *= scale.z;
  ret._33 *= scale.z;

  return ret;
}

void Decompose(in matrix mat, out float3 translation, out float3 scale, out quaternion rotation)
{
  translation = float3(mat._41, mat._42, mat._43);

  // scale is the length of each axis
  float scale_x = length(float3(mat[0][0], mat[0][1], mat[0][2]));
  float scale_y = length(float3(mat[1][0], mat[1][1], mat[1][2]));
  float scale_z = length(float3(mat[2][0], mat[2][1], mat[2][2]));

  // todo: why does this work?
  const float det = determinant(mat);
  if (det < 0) { scale_x = -scale_x; }

  // remove scale from matrix
  const matrix rotationMat = matrix
    (
     float4(mat[0][0] / scale_x, mat[0][1] / scale_x, mat[0][2] / scale_x, 0),
     float4(mat[1][0] / scale_y, mat[1][1] / scale_y, mat[1][2] / scale_y, 0),
     float4(mat[2][0] / scale_z, mat[2][1] / scale_z, mat[2][2] / scale_z, 0),
     float4(0, 0, 0, 1)
    );

  rotation = MatToQuaternion(rotationMat);

  scale = float3(scale_x, scale_y, scale_z);
}

#endif // __UTILITY_HLSLI__