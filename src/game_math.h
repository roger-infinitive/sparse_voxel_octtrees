#ifndef GAME_MATH_H
#define GAME_MATH_H

#include "utility.h"
#include <float.h>
#include <math.h>

#define PERCENTAGE(u) ((int)((u) * 100.0f))

#ifndef _TYPE_SERIALIZATION_H_

    #define WRITE();
    #define WRITE_ARRAY(count);
    #define SERIALIZE();

#endif

const float pi = 3.14159265358979323846;
#define DEG2RAD 0.017453292519943295f
#define TAU (2 * pi)
#define EPSILON 1e-6
#define SQUARE(f) ((f)*(f))
#define SWAP(f1, f2) { float temp = f1; f1 = f2; f2 = temp; }

int div2_to_zero_count_u32(u32 n) {
    if (n == 0) return 0;
    int count = 0;
    while (n) { n >>= 1; count++; }
    return count;
}

int div2_until_not_divisible_by4_u32(u32 n) {
    if (n == 0) return 0;

    int steps = 0;
    while ((n & 3u) == 0u) {
        n >>= 1;
        steps++;
    }
    return steps;
}

float srgb_to_linear(float cs) {
    if (cs <= 0.04045f) return cs / 12.92f;
    return powf((cs + 0.055f) / 1.055f, 2.4f);
}

float linear_to_srgb(float cl) {
    if (cl <= 0.0031308f) return cl * 12.92f;
    return 1.055f * powf(cl, 1.0f / 2.4f) - 0.055f;
}

bool is_power_of_two(u32 x) {
    return x != 0 && (x & (x - 1)) == 0;
}

u32 next_power_of_two(u32 x) {
    if (x == 0) { 
        return 1;
    }
    
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x + 1;
}

float u8_to_f01(u8 v) {
    return v * (1.0f / 255.0f);
}

u8 f01_to_u8(float v) {
    if (v < 0) v = 0; 
    if (v > 1) v = 1;
    return (u8)lroundf(v * 255.0f);
}

int Abs(int x) {
    int mask = x >> 31;
    return (x ^ mask) - mask;
}

float Abs(float x) {
    union { float f; u32 i; } u = { x };
    u.i &= 0x7FFFFFFF;
    return u.f;
}

int Sign(int x) {
    return (x > 0) - (x < 0);
}

float Sign(float f) {
    return (f > 0.0f) - (f < 0.0f);
}

float Lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

float Remap(float value, float from1, float to1, float from2, float to2) {
    return (value - from1) / (to1 - from1) * (to2 - from2) + from2;
}

// NOTE(roger): This does an extra mod to handle both positive and negative integers without any branches.
int Wrap(int x, int max) {
    return (max + (x % max)) % max;
}

bool IsFinite(float f) {
    union {
        float f;
        uint32_t u;
    } bits = { f };

    uint32_t exponent = (bits.u >> 23) & 0xFF;
    return exponent != 0xFF; // 0xFF = all 1s exponent → NaN or Inf
}


float LerpAngle(float start, float end, float t) {
    float delta = end - start;

    if (delta > pi) {
        delta -= (pi*2);
    }

    if (delta < -pi) {
        delta += (pi*2);
    }

    return start + t * delta;
}

float FastInverseSqrt(float number) {
    const float threehalfs = 1.5F;

    float x2 = number * 0.5F;
    float y = number;
    // Interpret the bits of the float as an integer (bit-level representation)
    int i = *(int*)&y;
    // Magic number for initial guess
    i = 0x5f3759df - (i >> 1);
    // Reinterpret the bits as a float
    y = *(float*)&i;
    // One iteration of Newton's method for refinement
    y = y * (threehalfs - (x2 * y * y));

    return y;
}

// Ease-out function (cubic)
float EaseOut(float t) {
    return 1 - powf(1 - t, 3);
}

float EaseIn(float min, float max, float t) {
    return min - (t*t) * (max - min);
}

float EaseOutSin(float t) {
    return sinf(t * pi * 0.5f);
}

float EaseInQuint(float t) {
    return t*t*t*t*t;
}

float EaseOutQuint(float t) {
    return 1 - powf(1 - t, 5);
}

int Min(u32 a, u32 b) {
    return a < b ? a : b;
}

int Max(u32 a, u32 b) {
    return a > b ? a : b;
}

int Min(int a, int b) {
    return a < b ? a : b;
}

int Max(int a, int b) {
    return a > b ? a : b;
}

float Min(float a, float b) {
    return a < b ? a : b;
}

float Max(float a, float b) {
    return a > b ? a : b;
}

bool IsFloatBetween(float min, float max, float c) {
    return (c >= min && c <= max) || (c >= max && c <= min);
}

bool IsFloatBetweenWithTolerance(float a, float b, float value, float tolerance) {
    return (Min(a, b) - tolerance <= value) && (value <= Max(a, b) + tolerance);
}

float DegreesToRadians(float degrees) {
    return degrees * (pi / 180.0f);
}

float RadianToDegrees(float radian) {
    return radian * (180.0f / pi);
}

bool Overlapping(float minA, float maxA, float minB, float maxB) {
    return minB <= maxA && minA <= maxB;
}

float gaussian(float x, float mu, float sigma) {
    float exponent = -((x - mu) * (x - mu)) / (2 * sigma * sigma);
    return (1.0 / (sigma * sqrt(2 * pi))) * exp(exponent);
}

float curve(float x, float mu, float sigma) {
    float exponent = -0.5f * ((x-mu)/sigma) * ((x-mu)/sigma);
    return exp(exponent);
}

bool FloatsEqual(float a, float b) {
    return Abs(a - b) < EPSILON;
}

struct VectorU16 {
    u16 x, y;
};

bool operator==(VectorU16 a, VectorU16 b) {
    return a.x == b.x && a.y == b.y;
}

bool operator!=(VectorU16 a, VectorU16 b) {
    return a.x != b.x || a.y != b.y;
}

struct Vector2Int {
    int x, y;
};

inline Vector2Int WrapCoord(Vector2Int coord, Vector2Int dimensions) {
    return { Wrap(coord.x, dimensions.x), Wrap(coord.y, dimensions.y) }; 
}

Vector2Int operator+(Vector2Int a, Vector2Int b) {
    return { a.x + b.x, a.y + b.y };
}

Vector2Int operator-(Vector2Int a, Vector2Int b) {
    return { a.x - b.x, a.y - b.y };
}

Vector2Int operator*(Vector2Int a, int b) {
    return { a.x * b, a.y * b };
}

bool operator==(Vector2Int a, Vector2Int b) {
    return a.x == b.x && a.y == b.y;
}

Vector2Int& operator+=(Vector2Int& a, const Vector2Int& b) {
    a.x += b.x;
    a.y += b.y;
    return a;
}

Vector2Int& operator-=(Vector2Int& a, const Vector2Int& b) {
    a.x -= b.x;
    a.y -= b.y;
    return a;
}

int Dot(Vector2Int a, Vector2Int b) {
    return a.x * b.x + a.y * b.y;
}

int Cross(Vector2Int a, Vector2Int b) {
    return a.x * b.y - a.y * b.x;
}

struct Vector2 {
    union {
        struct { float x, y; };
        float v[2];
    };

    float operator[](u32 index) { return v[index]; }
    Vector2& operator*=(float scalar);
    Vector2& operator/=(float scalar);
};

Vector2 InitVector2(Vector2Int v) {
    return { (float)v.x, (float)v.y };
}

Vector2 operator+(Vector2 a, Vector2 b) {
    return { a.x + b.x, a.y + b.y };
}

Vector2 operator-(Vector2 a, Vector2 b) {
    return { a.x - b.x, a.y - b.y };
}

bool operator==(Vector2 a, Vector2 b) {
    return FloatsEqual(a.x, b.x) && FloatsEqual(a.y, b.y);
}

Vector2& operator+=(Vector2& a, const Vector2& b) {
    a.x += b.x;
    a.y += b.y;
    return a;
}

Vector2& operator-=(Vector2& a, const Vector2& b) {
    a.x -= b.x;
    a.y -= b.y;
    return a;
}

Vector2 operator*(Vector2 a, float scalar) {
    return { a.x * scalar, a.y * scalar };
}

Vector2 &Vector2::operator*=(float scalar) {
    return *this = *this * scalar;
}

Vector2 operator/(Vector2 a, float scalar) {
    return { a.x / scalar, a.y / scalar };
}

Vector2 &Vector2::operator/=(float scalar) {
    return *this = *this / scalar;
}

Vector2 Negate(Vector2 v) {
    return { -v.x, -v.y };
}

Vector2 Lerp(Vector2 a, Vector2 b, float t) {
    Vector2 result;
    result.x = a.x + t * (b.x - a.x);
    result.y = a.y + t * (b.y - a.y);
    return result;
}

float SqrMagnitude(Vector2 v) {
    return v.x * v.x + v.y * v.y;
}

float Magnitude(Vector2 v) {
    return sqrt(v.x * v.x + v.y * v.y);
}

float FastMagnitude(Vector2 v) {
    return 1 / FastInverseSqrt(v.x * v.x + v.y * v.y);
}

Vector2 Normalize(Vector2 v) {
    float length = Magnitude(v);
    if (length > EPSILON) {
        return { v.x / length, v.y / length };
    }
    return v;
}

Vector2 Midpoint(Vector2 a, Vector2 b) {
    return { (a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f };
}

const Vector2 Vector2_Zero = { 0, 0 };
const Vector2 Vector2_One = { 1, 1 };
const Vector2 Vector2_Up = { 0, 1 };
const Vector2 Vector2_Right = { 1, 0 };
const Vector2 Vector2_Down = { 0, -1 };
const Vector2 Vector2_Left = { -1, 0 };

struct Vector3Int {
    int x, y, z;
};

struct Vector3 {
    union {
        struct { float x, y, z; };
        struct { float r, g, b; };
        float v[3];
    };
    
    const float& operator[](u32 index) const { return v[index]; }
};

Vector3 operator*(Vector3 a, float scalar) {
    return { a.x * scalar, a.y * scalar, a.z * scalar };
}

Vector3 RotateY(Vector3 v, float theta) {
    float s = sinf(theta);
    float c = cosf(theta);

    Vector3 r;
    r.x = v.x * c - v.z * s;
    r.y = v.y;
    r.z = v.x * s + v.z * c;
    return r;
}

struct Vector4 {
    union {
        struct { float x, y, z, w; };
        struct { float r, g, b, a; };
        float v[4];
    };

    const float& operator[](u32 index) const { return v[index]; }
    Vector4 &operator*=(float scalar);
};

Vector3 Lerp(Vector3 a, Vector3 b, float t) {
    Vector3 result;
    result.x = a.x + t * (b.x - a.x);  // Linear interpolation for hue
    result.y = a.y + t * (b.y - a.y);  // Lerp saturation
    result.z = a.z + t * (b.z - a.z);  // Lerp value
    return result;
}

float Magnitude(Vector3 v) {
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

Vector3 Normalize(Vector3 v) {
    float length = Magnitude(v);
    if (length > 0) {
        return { v.x / length, v.y / length, v.z / length };
    }
    return v;
}

Vector4 Lerp(Vector4 a, Vector4 b, float t) {
    Vector4 result;
    result.x = a.x + t * (b.x - a.x);
    result.y = a.y + t * (b.y - a.y);
    result.z = a.z + t * (b.z - a.z);
    result.w = a.w + t * (b.w - a.w);
    return result;
}

// Colors
const Vector4 WHITE       {1, 1, 1, 1};
const Vector4 BLACK       {0, 0, 0, 1};
const Vector4 LIGHT_GREY  {0.75f, 0.75f, 0.75f, 1};
const Vector4 DARK_GREY   {0.25f, 0.25f, 0.25f, 1};
const Vector4 GREY        {0.5f, 0.5f, 0.5f, 1};
const Vector4 RED         {1, 0, 0, 1};
const Vector4 DARK_RED    {0.5f, 0, 0, 1};
const Vector4 GREEN       {0, 1, 0, 1};
const Vector4 BLUE        {0, 0, 1, 1};
const Vector4 CYAN        {0, 1, 1, 1};
const Vector4 ORANGE      {1, 0.5f, 0, 1};
const Vector4 YELLOW      {1, 1, 0, 1};

const Vector4 HSV_RED     {0.0f,   1.0f, 1.0f, 1.0f};
const Vector4 HSV_CYAN    {0.5f,   1.0f, 1.0f, 1.0f};
const Vector4 HSV_ORANGE  {0.083f, 1.0f, 1.0f, 1.0f};
const Vector4 HSV_YELLOW  {0.166f, 1.0f, 1.0f, 1.0f};
const Vector4 HSV_WHITE   {0,      0,    1.0f, 1.0f};

// The line starts at the base point and extends infinitely in both directions.
struct Line {
    Vector2 base;
    Vector2 direction;
};

struct LineCoefficients {
    float a, b, c;
};

//A line segment is composed of a start and end point. They are not infinite.
SERIALIZE() struct LineSegment {
    WRITE() Vector2 point1;
    WRITE() Vector2 point2;
};

SERIALIZE() struct Circle {
    WRITE() Vector2 center;
    WRITE() float radius;
};

//An axis-aligned bounding box (or non-oriented Rectangle).
struct AABB {
    Vector2 origin;
    Vector2 size;
};

Vector2 Center(AABB bounds) {
    return bounds.origin + bounds.size * 0.5f;
}

SERIALIZE() struct OrientedRectangle {
    WRITE() Vector2 center;
    WRITE() Vector2 extents; //half size
    WRITE() float rotation;  //radians
};

struct Range {
    float min;
    float max;
};

struct Rect {
	float x, y;
	float hw, hh;
};

struct Transform2D {
    Vector2 position;
    Vector2 scale;
    float rotation;
};

int SutherlandHodgmanClip(AABB clip, OrientedRectangle obb, Vector2* output);

Vector3 operator+(Vector3 a, Vector3 b) {
    return { a.x + b.x, a.y + b.y, a.z + b.z };
}

Vector3 operator-(Vector3 a, Vector3 b) {
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}

Vector4 operator+(Vector4 a, Vector4 b) {
    return { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
}

Vector4 operator-(Vector4 a, Vector4 b) {
    return { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
}

Vector4 operator*(Vector4 a, float scalar) {
    return { a.x * scalar, a.y * scalar, a.z * scalar, a.w * scalar };
}

Vector4& operator+=(Vector4& a, const Vector4& b) {
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    a.w += b.w;
    return a;
}

Vector4& operator-=(Vector4& a, const Vector4& b) {
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
    a.w -= b.w;
    return a;
}

Vector4 &Vector4::operator*=(float scalar) {
    return *this = *this * scalar;
}

// Inclusive range
int RandomInt(int min, int max) {
    return min + rand() % (max - min + 1);
}

float RandomFloat(float min, float max) {
    return min + (float)rand() / ((float)RAND_MAX / (max - min));
}

float RandomFloat(Vector2 range) {
    return range.x + (float)rand() / ((float)RAND_MAX / (range.y - range.x));
}

// Returns a float in [min, max), skewed toward min.
// 'skew' > 1.0 makes it more strongly biased toward min (try 2.0 to 5.0).
float RandomWeightedRange(float min, float max, float skew) {
    float u = (float)rand() / ((float)RAND_MAX + 1); // [0,1)
    float weighted = powf(u, skew); // Bias toward 0 (min)
    return min + (max - min) * weighted;
}

// Returns a float between -1 and 1
float RandomSigned() {
    return (rand() / (float)RAND_MAX * 2.0f - 1.0f);
}

// Returns either -1 or 1
float RandomSignedInt() {
    return (rand() % 2) * 2 - 1;
}

Vector2 RandomVector2(float minX, float maxX, float minY, float maxY) {
    Vector2 vec;
    vec.x = minX + (float)rand() / ((float)RAND_MAX / (maxX - minX));
    vec.y = minY + (float)rand() / ((float)RAND_MAX / (maxY - minY));
    return vec;
}

Vector2 RandomPointInsideAABB(AABB box) {
    Vector2 max = box.origin + box.size;
    Vector2 vec;
    vec.x = box.origin.x + (float)rand() / ((float)RAND_MAX / (max.x - box.origin.x));
    vec.y = box.origin.y + (float)rand() / ((float)RAND_MAX / (max.y - box.origin.y));
    return vec;
}

Vector2 RandomPointInsideUnitCircle() {
    float theta = 2.0f * pi * ((float)rand() / (float)RAND_MAX);
    float r = sqrt((float)rand() / (float)RAND_MAX);
    Vector2 p;
    p.x = r * cosf(theta);
    p.y = r * sinf(theta);
    return p;
}

Vector2 RandomUnitVector2() {
    float angle = ((float)rand() / RAND_MAX) * 2.0f * pi;
    Vector2 vec;
    vec.x = cos(angle);
    vec.y = sin(angle);
    return vec;
}

Vector2 RotateRadians(Vector2 v, float radian) {
    float sine = sinf(radian);
    float cosine = cosf(radian);
    Vector2 r;
    r.x = v.x * cosine - v.y * sine;
    r.y = v.x * sine + v.y * cosine;
    return r;
}

Vector2 Rotate(Vector2 v, float degrees) {
    float radian = DegreesToRadians(degrees);
    float sine = sinf(radian);
    float cosine = cosf(radian);

    Vector2 r;
    r.x = v.x * cosine - v.y * sine;
    r.y = v.x * sine + v.y * cosine;
    return r;
}

Vector2 RotatePointAroundPivot(Vector2 point, Vector2 pivot, float degrees) {
    // Translate point to origin
    Vector2 translatedPoint = { point.x - pivot.x, point.y - pivot.y };
    // Rotate the point
    Vector2 rotatedPoint = Rotate(translatedPoint, degrees);
    // Translate point back to its original position
    return { rotatedPoint.x + pivot.x, rotatedPoint.y + pivot.y };
}

Vector2 Rotate90(Vector2 v) {
    Vector2 r = {-v.y, v.x};
    return r;
}

Vector2 Perpendicular(Vector2 v) {
    Vector2 r = {v.y, v.x};
    return r;
}

float RoundToHalf(float value) {
    int scaled = (int)(value * 2.0f);
    float rounded = ((scaled % 2) == 0) ? (float)scaled : (float)(scaled + 1);
    return rounded / 2.0f;
}

Vector2 AlignToNearestHalfPoint(Vector2 vec) {
    Vector2 result;
    result.x = RoundToHalf(vec.x);
    result.y = RoundToHalf(vec.y);
    return result;
}

Vector2 WorldToLocalPoint(Vector2 parentWorld, Vector2 childWorld) {
	return { childWorld.x - parentWorld.x, childWorld.y - parentWorld.y };
}

//When the dot product is == 0 then the angle between the vectors is 90 degrees.
//When the dot product is positive then the angle between the vectors is less than 90 degrees.
//When the dot product is negative then the angle between the vector is greater than 90 degrees.
float DotProduct(Vector2 a, Vector2 b) {
    return a.x * b.x + a.y * b.y;
}

float DotProduct(Vector3 a, Vector3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

float CrossProduct(Vector2 a, Vector2 b) {
    return (a.x * b.y) - (a.y * b.x);
}

Vector3 CrossProduct(Vector3 a, Vector3 b) {
    Vector3 r = {0};
    r.x = a.y * b.z - a.z * b.y;
    r.y = a.z * b.x - a.x * b.z;
    r.z = a.x * b.y - a.y * b.x;
    return r;
}

Vector2 TripleProduct(Vector2 a, Vector2 b, Vector2 c) {
    float ac = a.x * c.x + a.y * c.y;
    float bc = b.x * c.x + b.y * c.y;
    Vector2 r;
    r.x = b.x * ac - a.x * bc;
    r.y = b.y * ac - a.y * bc;
    return r;
}

Vector2 AveragePoint(Vector2* vertices, u32 count) {
    Vector2 average = {0};
    for (u32 i = 0; i < count; i++) {
        average = average + vertices[i];
    }
    average *= (1.0f / count);
    return average;
}

bool AreParallel(Vector2 a, Vector2 b) {
    Vector2 na = Rotate90(a);
    return FloatsEqual(0, DotProduct(na, b));
}

Vector2 Project(Vector2 a, Vector2 b) {
    float sqrDist = SqrMagnitude(b);
    if (0 < sqrDist) {
        float dot = DotProduct(a, b);
        return b * (dot/sqrDist);
    }
    return b;
}

struct Matrix4 {
    union {
        float f[16];
        float m[4][4];
        Vector4 r[4];
    };

    const Vector4& operator[](u32 index) const { return r[index]; }
};

const Matrix4 IdentityMatrix = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
};

Matrix4 Transpose(const Matrix4& m) {
    Matrix4 t = {};
    
    t.m[0][0] = m.m[0][0];
    t.m[0][1] = m.m[1][0];
    t.m[0][2] = m.m[2][0];
    t.m[0][3] = m.m[3][0];

    t.m[1][0] = m.m[0][1];
    t.m[1][1] = m.m[1][1];
    t.m[1][2] = m.m[2][1];
    t.m[1][3] = m.m[3][1];

    t.m[2][0] = m.m[0][2];
    t.m[2][1] = m.m[1][2];
    t.m[2][2] = m.m[2][2];
    t.m[2][3] = m.m[3][2];
    
    t.m[3][0] = m.m[0][3];
    t.m[3][1] = m.m[1][3];
    t.m[3][2] = m.m[2][3];
    t.m[3][3] = m.m[3][3];

    return t;
}

Vector4 operator*(const Vector4& v, const Matrix4& m) {
    Vector4 r = v;

    r.x = (v.x * m[0][0]) + (v.y * m[1][0]) + (v.z * m[2][0]) + (v.w * m[3][0]);
    r.y = (v.x * m[0][1]) + (v.y * m[1][1]) + (v.z * m[2][1]) + (v.w * m[3][1]);
    r.z = (v.x * m[0][2]) + (v.y * m[1][2]) + (v.z * m[2][2]) + (v.w * m[3][2]);
    r.w = (v.x * m[0][3]) + (v.y * m[1][3]) + (v.z * m[2][3]) + (v.w * m[3][3]);

    return r;
}

Matrix4 operator*(const Matrix4& a, const Matrix4& b) {
    Matrix4 r;

    r.m[0][0] = (a[0][0] * b[0][0]) + (a[0][1] * b[1][0]) + (a[0][2] * b[2][0]) + (a[0][3] * b[3][0]);
    r.m[0][1] = (a[0][0] * b[0][1]) + (a[0][1] * b[1][1]) + (a[0][2] * b[2][1]) + (a[0][3] * b[3][1]);
    r.m[0][2] = (a[0][0] * b[0][2]) + (a[0][1] * b[1][2]) + (a[0][2] * b[2][2]) + (a[0][3] * b[3][2]);
    r.m[0][3] = (a[0][0] * b[0][3]) + (a[0][1] * b[1][3]) + (a[0][2] * b[2][3]) + (a[0][3] * b[3][3]);

    r.m[1][0] = (a[1][0] * b[0][0]) + (a[1][1] * b[1][0]) + (a[1][2] * b[2][0]) + (a[1][3] * b[3][0]);
    r.m[1][1] = (a[1][0] * b[0][1]) + (a[1][1] * b[1][1]) + (a[1][2] * b[2][1]) + (a[1][3] * b[3][1]);
    r.m[1][2] = (a[1][0] * b[0][2]) + (a[1][1] * b[1][2]) + (a[1][2] * b[2][2]) + (a[1][3] * b[3][2]);
    r.m[1][3] = (a[1][0] * b[0][3]) + (a[1][1] * b[1][3]) + (a[1][2] * b[2][3]) + (a[1][3] * b[3][3]);

    r.m[2][0] = (a[2][0] * b[0][0]) + (a[2][1] * b[1][0]) + (a[2][2] * b[2][0]) + (a[2][3] * b[3][0]);
    r.m[2][1] = (a[2][0] * b[0][1]) + (a[2][1] * b[1][1]) + (a[2][2] * b[2][1]) + (a[2][3] * b[3][1]);
    r.m[2][2] = (a[2][0] * b[0][2]) + (a[2][1] * b[1][2]) + (a[2][2] * b[2][2]) + (a[2][3] * b[3][2]);
    r.m[2][3] = (a[2][0] * b[0][3]) + (a[2][1] * b[1][3]) + (a[2][2] * b[2][3]) + (a[2][3] * b[3][3]);

    r.m[3][0] = (a[3][0] * b[0][0]) + (a[3][1] * b[1][0]) + (a[3][2] * b[2][0]) + (a[3][3] * b[3][0]);
    r.m[3][1] = (a[3][0] * b[0][1]) + (a[3][1] * b[1][1]) + (a[3][2] * b[2][1]) + (a[3][3] * b[3][1]);
    r.m[3][2] = (a[3][0] * b[0][2]) + (a[3][1] * b[1][2]) + (a[3][2] * b[2][2]) + (a[3][3] * b[3][2]);
    r.m[3][3] = (a[3][0] * b[0][3]) + (a[3][1] * b[1][3]) + (a[3][2] * b[2][3]) + (a[3][3] * b[3][3]);

    return r;
}

Matrix4 MatrixTransformAroundPivot(Vector2 position, Vector2 scale, float rotation, Vector2 pivot = { 0.5f, 0.5f }) {
    float cosR = cosf(rotation);
    float sinR = sinf(rotation);

    float px = pivot.x * scale.x;
    float py = pivot.y * scale.y;

    float tx = position.x - (cosR * px - sinR * py);
    float ty = position.y - (sinR * px + cosR * py);

    Matrix4 m = {
        cosR * scale.x, sinR * scale.x, 0, 0,
       -sinR * scale.y, cosR * scale.y, 0, 0,
        0,              0,              1, 0,
        tx,             ty,             0, 1
    };
    
    return m;
}

Matrix4 MatrixTranslation(float x, float y) {
    return {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        x, y, 0, 1
    };
}

Matrix4 MatrixTranslation(Vector2 translation) {
    return {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        translation.x, translation.y, 0, 1
    };
}

Matrix4 MatrixTranslation(Vector3 translation) {
    return {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        translation.x, translation.y, translation.z, 1
    };
}

Matrix4 MatrixTranslation(Vector4 translation) {
    return {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        translation.x, translation.y, translation.z, 1
    };
}

Matrix4 MatrixRotateZ(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return {
         c,  s,  0, 0,
        -s,  c,  0, 0,
         0,  0,  1, 0,
         0,  0,  0, 1
    };
}

Matrix4 MatrixScale(float scale) {
    return {
        scale,   0,       0,     0,
        0,       scale,   0,     0,
        0,       0,       scale, 0,
        0,       0,       0,     1
    };
}

Matrix4 MatrixScale(float x, float y) {
    return {
        x, 0, 0, 0,
        0, y, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
}

Matrix4 MatrixScale(Vector2 scale) {
    return {
        scale.x, 0,       0, 0,
        0,       scale.y, 0, 0,
        0,       0,       1, 0,
        0,       0,       0, 1
    };
}

Matrix4 MatrixScale(Vector3 scale) {
    return {
        scale.x, 0,       0,       0,
        0,       scale.y, 0,       0,
        0,       0,       scale.z, 0,
        0,       0,       0,       1
    };
}

Matrix4 Transform2DToMatrix4(Transform2D transform) {
    return MatrixScale(transform.scale) * MatrixRotateZ(transform.rotation) * MatrixTranslation(transform.position);
}

Matrix4 PerspectiveLH(float aspect, float fov, float nearZ, float farZ) {
    Matrix4 m = {0};

    float y = 1.0f / tanf(fov * 0.5f);
    float x = y / aspect;

    m.m[0][0] = x;
    m.m[1][1] = y;
    m.m[2][2] = farZ / (farZ - nearZ);
    m.m[2][3] = 1.0f;
    m.m[3][2] = (-nearZ * farZ) / (farZ - nearZ);
    m.m[3][3] = 0.0f;

    return m;
}


Matrix4 OrthographicLH(float viewWidth, float viewHeight, float nearZ, float farZ) {
    Matrix4 m = {};
    m.m[0][0] = 2.0f / viewWidth;
    m.m[1][1] = 2.0f / viewHeight;
    m.m[2][2] = 1.0f / (farZ - nearZ);
    m.m[3][2] = -nearZ / (farZ - nearZ);
    m.m[3][3] = 1.0f;
    return m;
}

Matrix4 OrthographicOffCenterLH(float left, float right, float bottom, float top, float nearZ, float farZ) {
    Matrix4 m = {};
    m.m[0][0] = 2.0f / (right - left);
    m.m[1][1] = 2.0f / (top - bottom);
    m.m[2][2] = 1.0f / (farZ - nearZ);
    m.m[3][0] = -(left + right) / (right - left);
    m.m[3][1] = -(top + bottom) / (top - bottom);
    m.m[3][2] = -nearZ / (farZ - nearZ);
    m.m[3][3] = 1.0f;
    return m;
}

Matrix4 LookToLH(const Vector3& eye, const Vector3& forward, const Vector3& up) {
    Vector3 zAxis = Normalize(forward);
    Vector3 xAxis = Normalize(CrossProduct(up, zAxis));
    Vector3 yAxis = CrossProduct(zAxis, xAxis);

    Matrix4 view;

    // set the rotation part of the view matrix.
    view.m[0][0] = xAxis.x;
    view.m[0][1] = yAxis.x;
    view.m[0][2] = zAxis.x;
    view.m[0][3] = 0.0f;

    view.m[1][0] = xAxis.y;
    view.m[1][1] = yAxis.y;
    view.m[1][2] = zAxis.y;
    view.m[1][3] = 0.0f;

    view.m[2][0] = xAxis.z;
    view.m[2][1] = yAxis.z;
    view.m[2][2] = zAxis.z;
    view.m[2][3] = 0.0f;

    // set the translation part
    view.m[3][0] = -DotProduct(xAxis, eye);
    view.m[3][1] = -DotProduct(yAxis, eye);
    view.m[3][2] = -DotProduct(zAxis, eye);
    view.m[3][3] = 1.0f;

    return view;
}

Vector2 ProjectPointOnLineSegment(Vector2 point, LineSegment l) {
   return Project(point - l.point1, l.point2 - l.point1) + l.point1;
}

bool InRange(Vector2 p, Vector2 min, Vector2 max) {
    return ((p.x >= min.x) && (p.x <= max.x) &&
            (p.y >= min.y) && (p.y <= max.y));
}

bool IsProjectedAABBInsideLineSegment(Vector2* aabbPoints, LineSegment l) {
    Vector2 min = { Min(l.point1.x, l.point2.x), Min(l.point1.y, l.point2.y) };
    Vector2 max = { Max(l.point1.x, l.point2.x), Max(l.point1.y, l.point2.y) };
    if (InRange(ProjectPointOnLineSegment(aabbPoints[0], l), min, max)) {
        return true;
    }
    if (InRange(ProjectPointOnLineSegment(aabbPoints[1], l), min, max)) {
        return true;
    }
    if (InRange(ProjectPointOnLineSegment(aabbPoints[2], l), min, max)) {
        return true;
    }
    if (InRange(ProjectPointOnLineSegment(aabbPoints[3], l), min, max)) {
        return true;
    }
    return false;
}

bool LinesEquivalent(Line a, Line b) {
    if (!AreParallel(a.direction, b.direction)) {
        return false;
    }

    Vector2 d = a.base - b.base;
    return AreParallel(d, a.direction);
}

Range SortRange(Range r) {
    Range sorted = r;
    if (r.min > r.max) {
        sorted.min  = r.max;
        sorted.max = r.min;
    }
    return sorted;
}

Range ProjectLineSegment(LineSegment s, Vector2 onto) {
    Vector2 n = Normalize(onto);
    Range r;
    r.min = DotProduct(n, s.point1);
    r.max = DotProduct(n, s.point2);
    return SortRange(r);
}

bool RangesOverlap(Range a, Range b) {
    return Overlapping(a.min, a.max, b.min, b.max);
}

Range RangeHull(Range a, Range b) {
    Range hull;
    hull.min = a.min < b.min ? a.min : b.min;
    hull.max = a.max > b.max ? a.max : b.max;
    return hull;
}

struct LineSegmentAABBIntersectionResult {
    int hits;
    Vector2 first;
    Vector2 second;
    Vector2 firstNormal;
    Vector2 secondNormal;
    float tEnter;
    float tExit;
};

Vector2 GetAABBCenter(AABB aabb) {
    return {aabb.origin.x + aabb.size.x/2, aabb.origin.y + aabb.size.y/2};
}

void DecomposeAABBIntoPoints(AABB aabb, Vector2* points) {
    points[0] = aabb.origin;
    points[1] = aabb.origin; points[1].x += aabb.size.x;
    points[2] = aabb.origin; points[2].y += aabb.size.y;
    points[3] = aabb.origin + aabb.size;
}

void DecomposeAABBIntoLineSegments(AABB aabb, LineSegment* segments) {
    Vector2 points[4];
    DecomposeAABBIntoPoints(aabb, points);

    segments[0] = { points[0], points[1] };
    segments[1] = { points[0], points[2] };
    segments[2] = { points[1], points[3] };
    segments[3] = { points[2], points[3] };
}

AABB MinkowskiDifference(AABB a, AABB b) {
    AABB result;
    result.origin.x = a.origin.x - (b.origin.x + b.size.x);
    result.origin.y = a.origin.y - (b.origin.y + b.size.y);
    result.size.x = a.size.x + b.size.x;
    result.size.y = a.size.y + b.size.y;
    return result;
}

Vector2 AABB_PenetrationVector(AABB a, AABB b) {
    AABB md = MinkowskiDifference(a, b);
    Vector2 max = md.origin + md.size;

    float min = Abs(md.origin.x);
    Vector2 result = {md.origin.x, 0};
    if (Abs(max.x) < min)  {
        min = Abs(max.x);
        result = {max.x, 0};
    }
    if (Abs(max.y) < min) {
        min = Abs(max.y);
        result = {0, max.y};
    }
    if (Abs(md.origin.y) < min) {
        min = Abs(md.origin.y);
        result = {0, md.origin.y};
    }

    return result;
}

bool AABB_Collide(AABB a, AABB b) {
    float minX = Max(a.origin.x, b.origin.x);
    float maxX = Min(a.origin.x + a.size.x, b.origin.x + b.size.x);
    float minY = Max(a.origin.y, b.origin.y);
    float maxY = Min(a.origin.y + a.size.y, b.origin.y + b.size.y);
    return minX < maxX && minY < maxY;
}

bool AABB_Collide(AABB a, AABB b, AABB* overlap) {
    float minX = Max(a.origin.x, b.origin.x);
    float maxX = Min(a.origin.x + a.size.x, b.origin.x + b.size.x);
    float minY = Max(a.origin.y, b.origin.y);
    float maxY = Min(a.origin.y + a.size.y, b.origin.y + b.size.y);
    if (minX < maxX && minY < maxY) {
        *overlap = { minX, minY, maxX - minX, maxY - minY };
        return true;
    } else {
        return false;
    }
}

AABB CircleToAABB(Circle c) {
    AABB aabb;

    aabb.origin.x = c.center.x - c.radius;
    aabb.origin.y = c.center.y - c.radius;

    aabb.size.x = c.radius * 2;
    aabb.size.y = c.radius * 2;

    return aabb;
}

bool Circle_Collide(Circle a, Circle b) {
    float radiusSum = a.radius + b.radius;
    Vector2 distance = a.center - b.center;
    return SqrMagnitude(distance) <= radiusSum*radiusSum;
}

Vector2 Circle_OverlapCenter(Circle a, Circle b) {
    float totalRadius = a.radius + b.radius;
    Vector2 overlapCenter = {0};
    //weigted by radii
    overlapCenter.x = (a.radius * b.center.x + b.radius * a.center.x) / totalRadius;
    overlapCenter.y = (a.radius * b.center.y + b.radius * a.center.y) / totalRadius;
    return overlapCenter;
}

bool FindLineIntersection(Line a, Line b, Vector2* intersection) {
    float determinant = CrossProduct(a.direction, b.direction);
    // If determinant is zero, lines are parallel therefore no intersection
    if (determinant == 0.0f) { return false; }

    Vector2 pointDist = b.base - a.base;
    float t = CrossProduct(pointDist, b.direction) / determinant;
    float u = CrossProduct(pointDist, a.direction) / determinant;
    *intersection = { a.base.x + t * a.direction.x, a.base.y + t * a.direction.y };
    return true;
}

//In case you do not care about the boolean result this is a simpler function you can use.
//Sometimes external functions know implicitly that there is an intersection, so the determinant will never be 0.
Vector2 LineIntersection(Line a, Line b) {
    float determinant = CrossProduct(a.direction, b.direction);
    if (determinant == 0.0f) { return {0, 0}; }

    Vector2 pointDist = b.base - a.base;
    float t = CrossProduct(pointDist, b.direction) / determinant;
    float u = CrossProduct(pointDist, a.direction) / determinant;
    return { a.base.x + t * a.direction.x, a.base.y + t * a.direction.y };
}

bool Line_Collide(Line a, Line b) {
    if (!AreParallel(a.direction, b.direction))
        return true;

    return LinesEquivalent(a, b);
}

LineCoefficients PrecomputeLineCoefficients(LineSegment s) {
    LineCoefficients lc;
    lc.a = s.point2.y - s.point1.y;
    lc.b = s.point1.x - s.point2.x;
    lc.c = (s.point2.x * s.point1.y) - (s.point1.x * s.point2.y);
    return lc;
}

int PointLine_SideTest(Vector2 point, LineCoefficients lc) {
    return (lc.a * point.x) + (lc.b * point.y) + lc.c;
}

int AABBLine_SideTest(Vector2* aabbPoints, LineCoefficients lc) {
    int currentSide = PointLine_SideTest(aabbPoints[0], lc);
    if (currentSide == 0) {
        return 0;
    }

    int nextSide = PointLine_SideTest(aabbPoints[1], lc);
    if (nextSide == 0 || currentSide * nextSide < 0) {
        return 0;
    }

    nextSide = PointLine_SideTest(aabbPoints[2], lc);
    if (nextSide == 0 || currentSide * nextSide < 0) {
        return 0;
    }

    nextSide = PointLine_SideTest(aabbPoints[3], lc);
    if (nextSide == 0 || currentSide * nextSide < 0) {
        return 0;
    }

    return currentSide;
}

bool OnOneSide(Line axis, LineSegment s) {
    Vector2 d1 = s.point1 - axis.base;
    Vector2 d2 = s.point2 - axis.base;
    Vector2 n = Rotate90(axis.direction);
    return DotProduct(n, d1) * DotProduct(n, d2) > 0;
}

AABB LineSegmentToAABB(LineSegment line) {
    AABB aabb;

    aabb.origin.x = Min(line.point1.x, line.point2.x);
    aabb.origin.y = Min(line.point1.y, line.point2.y);

    aabb.size.x = Abs(line.point2.x - line.point1.x);
    aabb.size.y = Abs(line.point2.y - line.point1.y);

    return aabb;
}

bool LineSegment_Collide(LineSegment a, LineSegment b) {
    Line axisA, axisB;

    axisA.base = a.point1;
    axisA.direction = a.point2 - a.point1;
    if (OnOneSide(axisA, b)) {
        return false;
    }

    axisB.base = b.point1;
    axisB.direction = b.point2 - b.point1;
    if (OnOneSide(axisB, a)) {
        return false;
    }

    //The lines are not parallel, so they intersect.
    if (!AreParallel(axisA.direction, axisB.direction))
        return true;

    //If the lines are parallel, then we need to check if the line segments overlap.
    Range rA = ProjectLineSegment(a, axisA.direction);
    Range rB = ProjectLineSegment(b, axisA.direction);
    return RangesOverlap(rA, rB);
}

LineSegment OrientedRectangleEdge(OrientedRectangle r, int nr) {
    Vector2 a = r.extents;
    Vector2 b = r.extents;
    
    switch (nr % 4) {
        case 0: { a.x = -a.x; } break;
        case 1: { b.y = -b.y; } break;
        
        case 2: {
            a.y = -a.y;
            b = Negate(b);
        } break;
        
        default: {
            a = Negate(a);
            b.x = -b.x;
        } break;
    }

    LineSegment edge;
    edge.point1 = RotateRadians(a, r.rotation) + r.center;
    edge.point2 = RotateRadians(b, r.rotation) + r.center;

    return edge;
}

bool SeparatingEdgeForOrientedRectangle(LineSegment axis, OrientedRectangle r) {
    Range axisRange, r0Range, r2Range, rProjection;
    LineSegment rEdge0 = OrientedRectangleEdge(r, 0);
    LineSegment rEdge2 = OrientedRectangleEdge(r, 2);
    Vector2 n = axis.point1 - axis.point2;

    axisRange = ProjectLineSegment(axis, n);
    r0Range = ProjectLineSegment(rEdge0, n);
    r2Range = ProjectLineSegment(rEdge2, n);
    rProjection = RangeHull(r0Range, r2Range);

    return !RangesOverlap(axisRange, rProjection);
}

bool OrientedRectangle_Collide(OrientedRectangle a, OrientedRectangle b) {
    LineSegment edge = OrientedRectangleEdge(a, 0);
    if (SeparatingEdgeForOrientedRectangle(edge, b)) {
        return false;
    }

    edge = OrientedRectangleEdge(a, 1);
    if (SeparatingEdgeForOrientedRectangle(edge, b)) {
        return false;
    }

    edge = OrientedRectangleEdge(b, 0);
    if (SeparatingEdgeForOrientedRectangle(edge, a)) {
        return false;
    }

    edge = OrientedRectangleEdge(b, 1);
    return !SeparatingEdgeForOrientedRectangle(edge, a);
}

bool IsPointInsideCircle(Circle c, Vector2 p) {
    Vector2 distance = c.center - p;
    return SqrMagnitude(distance) <= c.radius*c.radius;
}

bool CircleLine_Collide(Circle c, Line l) {
    Vector2 lc = c.center - l.base;
    Vector2 p = Project(lc, l.direction);
    Vector2 nearest = l.base + p;
    return IsPointInsideCircle(c, nearest);
}

bool CircleLineSegment_Collide(Circle c, LineSegment s) {
    if (IsPointInsideCircle(c, s.point1))
        return true;

    if (IsPointInsideCircle(c, s.point2))
        return true;

    Vector2 d = s.point2 - s.point1;
    Vector2 lc = c.center - s.point1;
    Vector2 p = Project(lc, d);
    Vector2 nearest = s.point1 + p;

    return IsPointInsideCircle(c, nearest) &&
        SqrMagnitude(p) <= SqrMagnitude(d) &&
        DotProduct(p, d) >= 0;
}

float Clamp01(float x) {
    if (x < 0.0f) {
        return 0.0f;
    } else if (x > 1.0f) {
        return 1.0f;
    }
    return x;
}

float ClampF(float x, float min, float max) {
    if (x < min) {
        return min;
    } else if (x > max) {
        return max;
    }
    return x;
}

int Clamp(int value, int min, int max) {
    if (value < min) {
        return min;
    } else if (value > max) {
        return max;
    }
    return value;
}

u32 Clamp(u32 value, u32 min, u32 max) {
    if (value < min) {
        return min;
    } else if (value > max) {
        return max;
    }
    return value;
}

Vector2 ClampOnAABB(Vector2 p, AABB r) {
    Vector2 clamp;
    clamp.x = ClampF(p.x, r.origin.x, r.origin.x + r.size.x);
    clamp.y = ClampF(p.y, r.origin.y, r.origin.y + r.size.y);
    return clamp;
}


float WrapAngle(float angle) {
    while (angle > 2*pi) angle -= 2.0f * pi;
    while (angle < 0)    angle += 2.0f * pi;
    return angle;
}

float CalculateEnclosedAngle(Vector2 a, Vector2 b) {
    Vector2 aNorm = Normalize(a);
    Vector2 bNorm = Normalize(b);
    float dot = DotProduct(aNorm, bNorm);
    return RadianToDegrees(acosf(dot));
}

float Angle(Vector2 direction) {
    float angle = 0;
    float magnitude = Magnitude(direction); 
    
    if (magnitude > 0.0f) {
        direction /= magnitude;
        angle = acosf(ClampF(direction.x, -1, 1));
        if (direction.y < 0) {
            angle = 2 * pi - angle;
        }
    }

    return angle;
}

Vector2 Direction(float angle) {
    return { cosf(angle), sinf(angle) };
}

bool CircleAABB_Collide(Circle c, AABB r) {
    Vector2 clamped = ClampOnAABB(c.center, r);
    return IsPointInsideCircle(c, clamped);
}

bool CircleAABB_Collide(Circle c, AABB r, Vector2* hitPoint) {
    Vector2 clamped = ClampOnAABB(c.center, r);
    *hitPoint = clamped;
    return IsPointInsideCircle(c, clamped);
}

bool CircleOrientedRectangle_Collide(Circle c, OrientedRectangle r) {
    //Both shapes are transformed into the oriented rectangle's local coordinate system.
    AABB lr = {{0, 0}, r.extents * 2};
    Circle lc = {{0, 0}, c.radius};
    Vector2 distance = c.center - r.center;
    distance = RotateRadians(distance, -r.rotation);
    lc.center = distance + r.extents;

    return CircleAABB_Collide(lc, lr);
}

bool CircleOrientedRectangle_Collide(Circle c, OrientedRectangle r, Vector2* hitPoint) {
    //Both shapes are transformed into the oriented rectangle's local coordinate system.
    AABB lr = {{0, 0}, r.extents * 2};
    Circle lc = {{0, 0}, c.radius};
    Vector2 distance = c.center - r.center;
    distance = RotateRadians(distance, -r.rotation);
    lc.center = distance + r.extents;
    if (CircleAABB_Collide(lc, lr, hitPoint)) {
        *hitPoint = RotateRadians((*hitPoint) - r.extents, r.rotation); // Reverse translation and rotate back
        *hitPoint = (*hitPoint) + r.center;  // Move back into world space
        return true;
    }
    return false;
}

bool LineAABB_Collide(Line l, AABB r) {
    Vector2 n = Rotate90(l.direction);

    Vector2 c1 = r.origin - l.base;
    Vector2 c2 = c1 + r.size;
    Vector2 c3 = {c2.x, c1.y};
    Vector2 c4 = {c1.x, c2.y};

    float dp1 = DotProduct(n, c1);
    float dp2 = DotProduct(n, c2);
    float dp3 = DotProduct(n, c3);
    float dp4 = DotProduct(n, c4);

    return (dp1 * dp2 <= 0) || (dp2 * dp3 <= 0) || (dp3 * dp4 <= 0);
}

bool LineSegmentAABB_Collide(LineSegment l, AABB r) {
    Line line;
    line.base = l.point1;
    line.direction = l.point2 - l.point1;
    if (!LineAABB_Collide(line, r)) {
        return false;
    }

    Range rRange, lRange;
    rRange.min = r.origin.x;
    rRange.max = r.origin.x + r.size.x;
    lRange.min = l.point1.x;
    lRange.max = l.point2.x;
    lRange = SortRange(lRange);
    if (!RangesOverlap(rRange, lRange)) {
        return false;
    }

    rRange.min = r.origin.y;
    rRange.max = r.origin.y + r.size.y;
    lRange.min = l.point1.y;
    lRange.max = l.point2.y;
    lRange = SortRange(lRange);
    return RangesOverlap(rRange, lRange);
}

bool IsPointInsideAABB(Vector2 point, AABB box) {
    return (point.x >= box.origin.x) && (point.x <= box.origin.x + box.size.x) &&
           (point.y >= box.origin.y) && (point.y <= box.origin.y + box.size.y);
}

bool IsPointInsideTriangle(Vector2 p, Vector2 a, Vector2 b, Vector2 c) {
    // Calculate vectors
    Vector2 v0 = c - a;
    Vector2 v1 = b - a;
    Vector2 v2 = p - a;

    // Compute dot products
    float dot00 = DotProduct(v0, v0);
    float dot01 = DotProduct(v0, v1);
    float dot02 = DotProduct(v0, v2);
    float dot11 = DotProduct(v1, v1);
    float dot12 = DotProduct(v1, v2);

    // Compute barycentric coordinates
    float denom = dot00 * dot11 - dot01 * dot01;
    if (denom == 0.0f) return false; // Avoid divide by zero (degenerate triangle)
    float invDenom = 1.0f / denom;

    float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

    // Check if point is in triangle
    return (u >= 0) && (v >= 0) && (u + v <= 1);
}

Vector2 ClosestPointOnLineToOrigin(Vector2 p1, Vector2 p2) {
    Vector2 ab = p2 - p1;
    Vector2 ao = p1 * -1;
    float t = DotProduct(ao, ab) / DotProduct(ab, ab);
    t = ClampF(t, 0, 1);
    Vector2 d = p1 + (ab * t);
    return d;
}

Vector2 AABBCorner(AABB r, int nr) {
    switch (nr % 4) {
        case 0: return r.origin;
        case 1: return r.origin + Vector2{ r.size.x, 0 };
        case 2: return r.origin + r.size;
        case 3: return r.origin + Vector2{ 0, r.size.y };
    }
    return r.origin;
}

Vector2 OrientedRectangleCorner(OrientedRectangle r, int nr) {
    Vector2 corner = r.extents;
    switch (nr % 4) {
        case 0:  { corner.x  = -corner.x;      } break;
        case 1:  { /* corner = r.extents; */   } break;
        case 2:  { corner.y  = -corner.y;      } break;
        default: { corner    = Negate(corner); } break;
    }
    corner = RotateRadians(corner, r.rotation);
    return corner + r.center;
}

float Minimum(float a, float b) {
    return a < b ? a : b;
}

float Maximum(float a, float b) {
    return a > b ? a : b;
}

AABB EnlargeAABBPoint(AABB r, Vector2 p) {
    AABB enlarged;
    enlarged.origin.x = Minimum(r.origin.x, p.x);
    enlarged.origin.y = Minimum(r.origin.y, p.y);
    enlarged.size.x = Maximum(r.origin.x + r.size.x, p.x);
    enlarged.size.y = Maximum(r.origin.y + r.size.y, p.y);
    enlarged.size = enlarged.size - enlarged.origin;
    return enlarged;
}

AABB OrientedRectangleToAABB(OrientedRectangle r) {
    AABB h = { r.center, {0, 0} };
    int nr;
    Vector2 corner;
    for (nr = 0; nr < 4; ++nr) {
        corner = OrientedRectangleCorner(r, nr);
        h = EnlargeAABBPoint(h, corner);
    }
    return h;
}

bool IsPointInsideOrientedRectangle(Vector2 point, OrientedRectangle r) {
    AABB lr;
    lr.origin = { 0, 0 };
    lr.size = r.extents * 2;

    Vector2 lp = point - r.center;
    lp = RotateRadians(lp, r.rotation);
    lp = lp + r.extents;

    return IsPointInsideAABB(lp, lr);
}

bool SeperatingAxisForAABB(LineSegment axis, LineSegment rEdgeA, LineSegment rEdgeB) {
    Vector2 n = axis.point1 - axis.point2;

    Range rEdgeARange = ProjectLineSegment(rEdgeA, n);
    Range rEdgeBRange = ProjectLineSegment(rEdgeB, n);
    Range rProjection = RangeHull(rEdgeARange, rEdgeBRange);
    
    Range axisRange   = ProjectLineSegment(axis, n);

    return RangesOverlap(axisRange, rProjection);
}

bool OrientedRectangleAABB_Collide(OrientedRectangle r1, AABB r2) {
    AABB orHull = OrientedRectangleToAABB(r1);
    if (!AABB_Collide(orHull, r2)) {
        return false;
    }
    
    LineSegment edge0;
    edge0.point1 = { -r1.extents.x, r1.extents.y };
    edge0.point1 = RotateRadians(edge0.point1, r1.rotation) + r1.center;
    edge0.point2 = RotateRadians(r1.extents, r1.rotation) + r1.center;

    LineSegment aabbEdgeA, aabbEdgeB;
    aabbEdgeA.point1 = AABBCorner(r2, 0);
    aabbEdgeA.point2 = AABBCorner(r2, 1);
    aabbEdgeB.point1 = AABBCorner(r2, 2);
    aabbEdgeB.point2 = AABBCorner(r2, 3);

    if (!SeperatingAxisForAABB(edge0, aabbEdgeA, aabbEdgeB)) {
        return false;
    }
    
    LineSegment edge1;
    edge1.point1 = RotateRadians(r1.extents, r1.rotation) + r1.center;
    edge1.point2 = { r1.extents.x, -r1.extents.y };
    edge1.point2 = RotateRadians(edge1.point2, r1.rotation) + r1.center;

    if (!SeperatingAxisForAABB(edge1, aabbEdgeA, aabbEdgeB)) {
        return false;
    }

    return true;
}

bool OrientedRectangleAABB_Collide(OrientedRectangle r1, AABB r2, Vector2* hitPoint) {
    if (!OrientedRectangleAABB_Collide(r1, r2)) {
        return false;
    }

    const u32 MAX_VERTICES = 16;
    Vector2 clipVertices[MAX_VERTICES];

    int clipCount = SutherlandHodgmanClip(r2, r1, clipVertices);
    Vector2 average = {0};
    for (u32 i = 0; i < clipCount; i++) {
        average += clipVertices[i];
    }
    *hitPoint = average * (1.0f / (float)clipCount);

    return true;
}

bool OrientedRectangleLineSegmentOverlap(OrientedRectangle r, LineSegment l) {
    AABB lr;
    lr.origin.x = 0;
    lr.origin.y = 0;
    lr.size = r.extents * 2;
    LineSegment ls;
    ls.point1 = RotateRadians(l.point1 - r.center, -r.rotation) + r.extents;
    ls.point2 = RotateRadians(l.point2 - r.center, -r.rotation) + r.extents;
    return LineSegmentAABB_Collide(ls, lr);
}

//TODO(roger): Refactor
bool LineSegmentAABB_Intersection(LineSegment line, AABB box, LineSegmentAABBIntersectionResult* result) {
    *result = {0};

    // First, check if both endpoints are inside the AABB
    bool point1Inside = IsPointInsideAABB(line.point1, box);
    bool point2Inside = IsPointInsideAABB(line.point2, box);

    // If both points are inside, return no intersection
    if (point1Inside && point2Inside) {
        result->hits = 0;
        return false;
    }

    // Direction of the line
    Vector2 dir = {line.point2.x - line.point1.x, line.point2.y - line.point1.y};

    // Handle zero-length direction vectors
    if (dir.x == 0 && dir.y == 0) {
        // Line segment is a point
        result->hits = 0;
        return false;
    }

    // Inverse direction components
    float invDirX = 1.0f / dir.x;
    float invDirY = 1.0f / dir.y;

    // Calculate tMin and tMax for each axis
    float tMinX = (box.origin.x - line.point1.x) * invDirX;
    float tMaxX = (box.origin.x + box.size.x - line.point1.x) * invDirX;
    if (tMinX > tMaxX) { SWAP(tMinX, tMaxX); }

    float tMinY = (box.origin.y - line.point1.y) * invDirY;
    float tMaxY = (box.origin.y + box.size.y - line.point1.y) * invDirY;
    if (tMinY > tMaxY) { SWAP(tMinY, tMaxY); }

    // Find the largest tMin and smallest tMax
    float tEnter = Max(tMinX, tMinY);
    float tExit = Min(tMaxX, tMaxY);

    // Check if the line intersects the AABB
    if (tEnter > tExit || tExit < 0 || tEnter > 1) {
        result->hits = 0;
        return false;
    }

    // Calculate entry point if tEnter is within [0,1]
    if (tEnter >= 0 && tEnter <= 1) {
        result->first.x = line.point1.x + tEnter * dir.x;
        result->first.y = line.point1.y + tEnter * dir.y;

        // Determine normal at the entry point
        if (tEnter == tMinX) {
            result->firstNormal = {(invDirX < 0) ? 1.0f : -1.0f, 0.0f};
        } else if (tEnter == tMinY) {
            result->firstNormal = {0.0f, (invDirY < 0) ? 1.0f : -1.0f};
        }
    }

    // Calculate exit point if tExit is within [0,1]
    if (tExit >= 0 && tExit <= 1 && tEnter != tExit) {
        result->second.x = line.point1.x + tExit * dir.x;
        result->second.y = line.point1.y + tExit * dir.y;

        // Determine normal at the exit point
        if (tExit == tMaxX) {
            result->secondNormal = {(invDirX < 0) ? -1.0f : 1.0f, 0.0f};
        } else if (tExit == tMaxY) {
            result->secondNormal = {0.0f, (invDirY < 0) ? -1.0f : 1.0f};
        }
    }

    // Determine the number of hits
    if (tEnter >= 0 && tEnter <= 1) {
        result->hits = (tExit >= 0 && tExit <= 1 && tEnter != tExit) ? 2 : 1;
    } else if (tExit >= 0 && tExit <= 1) {
        result->hits = 1;
        result->first = result->second;
        result->firstNormal = result->secondNormal;
    } else {
        result->hits = 0;
    }

    result->tEnter = tEnter;
    result->tExit = tExit;
    return true;
}

bool LineSegment_Intersection(LineSegment a, LineSegment b, Vector2* intersection) {
    float A1 = a.point2.y - a.point1.y;
    float B1 = a.point1.x - a.point2.x;
    float C1 = A1 * a.point1.x + B1 * a.point1.y;
    float A2 = b.point2.y - b.point1.y;
    float B2 = b.point1.x - b.point2.x;
    float C2 = A2 * b.point1.x + B2 * b.point1.y;
    float delta = A1 * B2 - A2 * B1;
    if (Abs(delta) < EPSILON) {
        return false; //Lines are parallel
    }
    intersection->x = (B2 * C1 - B1 * C2) / delta;
    intersection->y = (A1 * C2 - A2 * C1) / delta;
    bool isBetween = (IsFloatBetweenWithTolerance(a.point1.x, a.point2.x, intersection->x, EPSILON) &&
        IsFloatBetweenWithTolerance(a.point1.y, a.point2.y, intersection->y, EPSILON) &&
        IsFloatBetweenWithTolerance(b.point1.x, b.point2.x, intersection->x, EPSILON) &&
        IsFloatBetweenWithTolerance(b.point1.y, b.point2.y, intersection->y, EPSILON));
    return isBetween;
}

struct IntersectHit {
    float t;
    Vector2 point;
    Vector2 normal;
};
bool IntersectSegment(AABB box, LineSegment line, IntersectHit* hit) {
    Vector2 dir = {line.point2.x - line.point1.x, line.point2.y - line.point1.y};
    if (dir.x == 0 && dir.y == 0) {
        return false;
    }

    Line l;
    l.base = line.point1;
    l.direction = dir;
    if (!LineAABB_Collide(l, box)) {
        return false;
    }
    float minA = line.point1.x;
    float maxA = line.point2.x;
    float minB = box.origin.x;
    float maxB = box.origin.x + box.size.x;
    if (minA > maxA) { SWAP(minA, maxA); }
    if (minB > maxB) { SWAP(minB, maxB); }
    bool isOverlapping = (minB < maxA && minA < maxB);
    if (!isOverlapping) {
        return false;
    }
    minA = line.point1.y;
    maxA = line.point2.y;
    minB = box.origin.y;
    maxB = box.origin.y + box.size.y;
    if (minA > maxA) { SWAP(minA, maxA); }
    if (minB > maxB) { SWAP(minB, maxB); }
    isOverlapping = (minB < maxA && minA < maxB);
    if (!isOverlapping) {
        return false;
    }

    // Inverse direction components
    float invDirX = 1.0f / dir.x;
    float invDirY = 1.0f / dir.y;
    // Calculate tMin and tMax for each axis

    Vector2 originToPoint1 = box.origin - line.point1;
    Vector2 cornerToPoint1 = (box.origin + box.size) - line.point1;

    originToPoint1.x *= (Abs(originToPoint1.x) >= EPSILON);
    originToPoint1.y *= (Abs(originToPoint1.y) >= EPSILON);
    cornerToPoint1.x *= (Abs(cornerToPoint1.x) >= EPSILON);
    cornerToPoint1.y *= (Abs(cornerToPoint1.y) >= EPSILON);

    float tMinX = originToPoint1.x * invDirX;
    float tMaxX = cornerToPoint1.x * invDirX;
    if (tMinX > tMaxX) { SWAP(tMinX, tMaxX); }
    float tMinY = originToPoint1.y * invDirY;
    float tMaxY = cornerToPoint1.y * invDirY;
    if (tMinY > tMaxY) { SWAP(tMinY, tMaxY); }
    // Find the largest tMin and smallest tMax
    float tEnter = Max(tMinX, tMinY);
    float tExit = Min(tMaxX, tMaxY);
    // Check if the line intersects the AABB
    if (tEnter > tExit || tExit < 0 || tEnter > 1) { return false; }
    // Calculate entry point if tEnter is within [0,1]
    if (tEnter < -1e-5 || tEnter > 1) { return false; }
    if (tEnter < 0) { tEnter = 0.0f; }
    hit->t = tEnter;
    hit->point = line.point1 + (dir * tEnter);
    if (tEnter == tMinX) {
        hit->normal = {(invDirX < 0) ? 1.0f : -1.0f, 0.0f};
    } else if (tEnter == tMinY) {
        hit->normal = {0.0f, (invDirY < 0) ? 1.0f : -1.0f};
    }
    return true;
}

bool IntersectRayOrientedRectangle(Vector2 rayOrigin, Vector2 rayDir, OrientedRectangle obb, float* t, Vector2* normal) {
    AABB box;
    box.origin.x = 0;
    box.origin.y = 0;
    box.size = obb.extents * 2;

    LineSegment line;
    line.point1 = RotateRadians(rayOrigin - obb.center, -obb.rotation) + obb.extents;
    line.point2 = RotateRadians((rayOrigin + rayDir) - obb.center, -obb.rotation) + obb.extents;

    IntersectHit hit = {0};
    bool intersect = IntersectSegment(box, line, &hit);
    if (intersect) {
        *t = hit.t;
        *normal = hit.normal;
    }
    return intersect;
}

AABB CalculateSweptAABB(AABB aabb, Vector2 velocity) {
    Vector2 aabbMax = aabb.origin + aabb.size;

    AABB sweptAABB = aabb;
    sweptAABB.origin.x += Min(0.0f, velocity.x);
    sweptAABB.origin.y += Min(0.0f, velocity.y);
    Vector2 sweptMax = { Max(0.0f, velocity.x), Max(0.0f, velocity.y) };
    sweptMax += aabbMax;
    sweptAABB.size = sweptMax - sweptAABB.origin;
    
    return sweptAABB;
}

bool IsAxisAligned(Vector2 v) {
    Vector2 vx = {1, 0};
    Vector2 vy = {0, 1};

    float dotX = Abs(DotProduct(v, vx));
    float dotY = Abs(DotProduct(v, vy));

    return Abs(dotX - 1.0f) < EPSILON || Abs(dotY - 1.0f) < EPSILON;
}

struct TimeOfImpactInfo {
    bool hit;
    float timeOfImpact; // (0 to 1)
    Vector2 point;
    Vector2 normal;
    bool overlaps;
};

TimeOfImpactInfo SweepPoint(Vector2 point, Vector2 ray, AABB aabb) {
    TimeOfImpactInfo result = {};
    result.timeOfImpact = 1.0f;
    
    Vector2 max = aabb.origin + aabb.size;

    Vector2 invEntry, invExit;
    
    if (ray.x > 0.0f) {
        invEntry.x = aabb.origin.x - point.x;
        invExit.x  = max.x         - point.x;
    } else {
        invEntry.x = max.x         - point.x;
        invExit.x  = aabb.origin.x - point.x;
    }

    if (ray.y > 0.0f) {
        invEntry.y = aabb.origin.y - point.y;
        invExit.y  = max.y         - point.y;
    } else {
        invEntry.y = max.y         - point.y;
        invExit.y  = aabb.origin.y - point.y;
    }

    Vector2 entry, exit;
    entry.x = (ray.x == 0.0f) ? -INFINITY : invEntry.x / ray.x;
    exit.x  = (ray.x == 0.0f) ?  INFINITY : invExit.x  / ray.x;
    entry.y = (ray.y == 0.0f) ? -INFINITY : invEntry.y / ray.y;
    exit.y  = (ray.y == 0.0f) ?  INFINITY : invExit.y  / ray.y;

    float entryTime = Max(entry.x, entry.y);
    float exitTime  = Min(exit.x,  exit.y);

    if (entryTime > exitTime || entry.x < 0.0f && entry.y < 0.0f || entry.x > 1.0f || entry.y > 1.0f) {
        return result;
    }
    
    result.timeOfImpact = entryTime;
    result.hit = true;

    // Collision normal
    if (entry.x > entry.y) {
        result.normal.x = (invEntry.x < 0.0f) ? 1.0f : -1.0f;
    } else {
        result.normal.y = (invEntry.y < 0.0f) ? 1.0f : -1.0f;
    }
    
    result.point = point + (ray * entryTime);
    return result;
}

TimeOfImpactInfo SweepPoint(Vector2 point, Vector2 ray, LineSegment seg) {
    TimeOfImpactInfo hitInfo = {};
    hitInfo.timeOfImpact = 1.0f;
    
    LineSegment sweepLine;
    sweepLine.point1 = point;
    sweepLine.point2 = point + ray;

    Vector2 p = sweepLine.point1;
    Vector2 r = sweepLine.point2 - sweepLine.point1;
    Vector2 q = seg.point1;
    Vector2 s = seg.point2 - seg.point1;

    float rxs = r.x * s.y - r.y * s.x;
    if (Abs(rxs) < 1e-6f) { 
        return hitInfo; // parallel
    }

    Vector2 qp = q - p;
    float t = (qp.x * s.y - qp.y * s.x) / rxs;
    float u = (qp.x * r.y - qp.y * r.x) / rxs;

    if (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f) {
        hitInfo.timeOfImpact = t;
        hitInfo.point = p + r * t;
        hitInfo.hit = true;
        hitInfo.normal = Normalize(Rotate90(s));
    }

    return hitInfo;
}

// ||center + (t * velocity) − circleCenter|| = r
// the formula is restructured into the quadratic form to solve t
TimeOfImpactInfo SweepPoint(Vector2 point, Vector2 velocity, Circle circle) {
    TimeOfImpactInfo hitInfo = {};
    hitInfo.timeOfImpact = 1.0f;

    Vector2 d = point - circle.center;
    float a = SqrMagnitude(velocity);
    float b = 2.0f * DotProduct(velocity, d);
    float c = SqrMagnitude(d) - (circle.radius * circle.radius);
    float disc = b*b - 4*a*c;
    
    if (disc >= 0.0f) {
        float t = (-b - sqrtf(disc)) / (2*a);
        if (t >= 0.0f && t <= 1.0f) {
            Vector2 hitPoint = point + (velocity * t);
        
            hitInfo.timeOfImpact = t;
            hitInfo.point = hitPoint;
            hitInfo.normal = Normalize(hitPoint - circle.center); 
            hitInfo.hit = true;
        }
    }
    
    return hitInfo;
}

Vector2 ClosestPointOnSegment(Vector2 p, LineSegment line) {
    Vector2 segVec = line.point2 - line.point1;
    float segLenSq = DotProduct(segVec, segVec);

    if (segLenSq == 0.0f) return line.point1; // line is a point

    float t = DotProduct(p - line.point1, segVec) / segLenSq;
    t = Clamp01(t); // clamp to [0, 1]

    return line.point1 + segVec * t;
}

TimeOfImpactInfo SweepAABB(AABB aabb, Vector2 velocity, LineSegment line) {
    TimeOfImpactInfo result = {};
    result.timeOfImpact = 1.0f;
    
    float entryTime = 0.0f;
    float exitTime = 1.0f;
    Vector2 hitNormal;
    
    bool hit = false;
    
    // Y-Axis
    
    float aabbMin = aabb.origin.y;
    float aabbMax = aabb.origin.y + aabb.size.y;
    float lineMin = line.point1.y;
    float lineMax = line.point2.y;
    if (lineMin > lineMax) {
        SWAP(lineMin, lineMax);
    }
    
    float v = velocity.y;
    if (Abs(v) < EPSILON) {
        if (lineMax < aabbMin || lineMin > aabbMax) {
            return result;
        }
    } else {
        float t0 = (lineMin - aabbMax) / v;
        float t1 = (lineMax - aabbMin) / v;
        if (t0 > t1) {
            SWAP(t0, t1);
        }
        
        if (t0 > entryTime) {
            hit = true;
            entryTime = t0;
            hitNormal = {0, -Sign(v)};
        }
        
        exitTime = Min(t1, exitTime);
        if (entryTime > exitTime) {
            return result;            
        }
    }
    
    // X-Axis
    
    aabbMin = aabb.origin.x;
    aabbMax = aabb.origin.x + aabb.size.x;
    lineMin = line.point1.x;
    lineMax = line.point2.x;
    if (lineMin > lineMax) {
        SWAP(lineMin, lineMax);
    }
    
    v = velocity.x;
    if (Abs(v) < EPSILON) {
        if (lineMax < aabbMin || lineMin > aabbMax) {
            return result;
        }
    } else {
        float t0 = (lineMin - aabbMax) / v;
        float t1 = (lineMax - aabbMin) / v;
        if (t0 > t1) {
            SWAP(t0, t1);
        }
        
        if (t0 > entryTime) {
            hit = true;
            entryTime = t0;
            hitNormal = {-Sign(v), 0};
        }
        
        exitTime = Min(t1, exitTime);
        if (entryTime > exitTime) {
            return result;            
        }
    }
    
    // Line Normal Axis
    
    Vector2 normal = Rotate90(Normalize(line.point2 - line.point1));
    Vector2 aabbCorners[4] = {
        aabb.origin,
        aabb.origin + Vector2 { aabb.size.x, 0 },
        aabb.origin + Vector2 { 0, aabb.size.y },
        aabb.origin + aabb.size
    };
    
    aabbMin = DotProduct(aabbCorners[0], normal);
    aabbMax = aabbMin;
    
    for (int j = 1; j < countOf(aabbCorners); j++) {
        float proj = DotProduct(aabbCorners[j], normal);
        aabbMin = Min(aabbMin, proj);
        aabbMax = Max(aabbMax, proj);
    }
    
    float lineDot = DotProduct(line.point1, normal); 

    v = DotProduct(velocity, normal);
    if (Abs(v) < EPSILON) {
        if (lineDot < aabbMin || lineDot > aabbMax) {
            return result;
        }
    } else {
        float t0 = (lineDot - aabbMax) / v;
        float t1 = (lineDot - aabbMin) / v;
        if (t0 > t1) {
            SWAP(t0, t1);
        }
        
        if (t0 > entryTime) {
            hit = true;
            entryTime = t0;
            hitNormal = normal * -Sign(v);
        }
        
        exitTime = Min(t1, exitTime);
        if (entryTime > exitTime) {
            return result;            
        }
    }

    if (!hit || entryTime > 1.0f) {
        return result;
    }
    
    result.hit = true;
    result.timeOfImpact = entryTime;
    result.normal = hitNormal;

    // SPEED(roger): We don't really need to calculate a hit point for TimeOfImpact calculations.
    // There are physics in the game that are only concerned with moving a thing forward and making sure
    // it slides or bounces off that surface which means you only really need hitNormal and timeOfImpact.
    // However, we could move this to a separate function that can be used after detecting an impact to 
    // give us the closest hit point. We can do this for all time of impact functions. Essentially remove 
    // point from the TimeOfImpactInfo struct. 
    
    Vector2 displacement = velocity * entryTime;
    aabbCorners[0] += displacement;    
    aabbCorners[1] += displacement;    
    aabbCorners[2] += displacement;    
    aabbCorners[3] += displacement;    
        
    // Is the LineSegment axis aligned?
    if (IsAxisAligned(normal)) {
        Vector2 projAxis = Rotate90(hitNormal);
    
        float aabbMin, aabbMax;
        aabbMin = DotProduct(aabbCorners[0], projAxis);
        aabbMax = aabbMin;
        
        for (int j = 0; j < countOf(aabbCorners); j++) {
            float proj = DotProduct(aabbCorners[j], projAxis);
            aabbMin = Min(proj, aabbMin);
            aabbMax = Max(proj, aabbMax);
        }
        
        float lineMin = DotProduct(line.point1, projAxis);
        float lineMax = DotProduct(line.point2, projAxis);
        if (lineMin > lineMax) {
            SWAP(lineMin, lineMax);
        }
        
        // Overlapping range on this axis
        float overlapMin = Max(lineMin, aabbMin);
        float overlapMax = Min(lineMax, aabbMax);
        float overlapCenter = (overlapMin + overlapMax) * 0.5f;
    
        float dot1 = DotProduct(line.point1, hitNormal);
        float dot2 = DotProduct(line.point2, hitNormal);
        if (dot1 < dot2) {
            SWAP(dot1, dot2);
        }
            
        result.point = projAxis * overlapCenter + hitNormal * dot1;
    
    } else if (IsAxisAligned(hitNormal)) {
        float dot1 = DotProduct(line.point1, hitNormal);
        float dot2 = DotProduct(line.point2, hitNormal);
        if (dot1 > dot2) {
            result.point = line.point1;
        } else {
            result.point = line.point2;
        }
        
    } else {
        float min = DotProduct(aabbCorners[0], hitNormal);
        result.point = aabbCorners[0];
        
        for (int j = 0; j < countOf(aabbCorners); j++) {
            float proj = DotProduct(aabbCorners[j], hitNormal);
            if (proj < min) {
                min = proj;
                result.point = aabbCorners[j];
            }
        }
    }
    
    return result;
}

// Use minkowski to reduce 'a' down to a single point and ray against 'b'. 
//      This produces an accurate hit point. 
// Use "slab" tests on the X and Y axis independently.
// Compute entry and exit times for X and Y.
// Compute if a hit occured and return info about the hit result.
TimeOfImpactInfo SweepAABB(AABB a, Vector2 velocity, AABB b) {
    Vector2 halfSize = a.size * 0.5f;
    Vector2 aPos = a.origin + halfSize;
    
    AABB expandedAABB = b;
    expandedAABB.origin -= halfSize;
    expandedAABB.size   += halfSize * 2.0f;
    
    TimeOfImpactInfo result = SweepPoint(aPos, velocity, expandedAABB);
    if (!result.hit) {
        return result;
    }
    
    // Contact point
    if (result.normal.x != 0) {
        result.point.x = (result.normal.x > 0)
            ? b.origin.x + b.size.x
            : b.origin.x;
    
        result.point.y = ClampF(result.point.y, b.origin.y, b.origin.y + b.size.y);
    }
    
    if (result.normal.y != 0) {
        result.point.y = (result.normal.y > 0)
            ? b.origin.y + b.size.y
            : b.origin.y;
    
        result.point.x = ClampF(result.point.x, b.origin.x, b.origin.x + b.size.x);
    }
    
    return result;
}

TimeOfImpactInfo SweepCircle(Circle c1, Vector2 velocity, Circle c2) {
    TimeOfImpactInfo hitInfo = {};
    hitInfo.timeOfImpact = 1.0f;

    // minkowski
    Vector2 point = c1.center;
    c2.radius += c1.radius;

    Vector2 d = point - c2.center;
    float a = SqrMagnitude(velocity);
    float b = 2.0f * DotProduct(velocity, d);
    float c = SqrMagnitude(d) - SQUARE(c2.radius);
    float disc = b*b - 4*a*c;
    
    if (disc >= 0.0f) {
        float t = (-b - sqrtf(disc)) / (2*a);
        if (t >= 0.0f && t <= 1.0f) {
            Vector2 hitPoint = point + (velocity * t);
            Vector2 normal = Normalize(hitPoint - c2.center);
            hitPoint -= normal * c1.radius;
        
            hitInfo.timeOfImpact = t;
            hitInfo.point = hitPoint;
            hitInfo.normal = normal; 
            hitInfo.hit = true;
        }
    }
    
    return hitInfo;
}

// Using minkowski sum, this collapses the circle down to a single point, and treats the line as a capsule.
TimeOfImpactInfo SweepCircle(Circle circle, Vector2 velocity, LineSegment line) {
    TimeOfImpactInfo hitInfo = {};
    hitInfo.timeOfImpact = 1.0f;

    // Ray vs. capsule body
    Vector2 segmentVector = line.point2 - line.point1; 
    Vector2 segmentNormal = Rotate90(Normalize(segmentVector));
    
    float v = DotProduct(velocity, segmentNormal);
    if (Abs(v) > EPSILON) {
        float dist = DotProduct(line.point1 - circle.center, segmentNormal) + (circle.radius * -Sign(v));
        float t = dist / v;
        
        if (t >= 0.0f && t <= 1.0f) {
            Vector2 circlePoint = circle.center + (velocity * t); 
            Vector2 toHit = circlePoint - line.point1;
            float projectedTime = DotProduct(toHit, segmentVector) / DotProduct(segmentVector, segmentVector);
            if (projectedTime >= 0 && projectedTime <= 1.0f) {
                hitInfo.timeOfImpact = t;
                hitInfo.point  = line.point1 + segmentVector * projectedTime;
                hitInfo.normal = segmentNormal * -Sign(v);
                hitInfo.hit = true;
            }
        }
    }
    
    // Ray vs. end-cap circle at line.point1
    {
        Circle end;
        end.center = line.point1;
        end.radius = circle.radius;
        TimeOfImpactInfo endCapHitInfo = SweepPoint(circle.center, velocity, end);
        if (endCapHitInfo.hit && endCapHitInfo.timeOfImpact < hitInfo.timeOfImpact) {
            hitInfo = endCapHitInfo;
            hitInfo.point = line.point1;
            Vector2 centerPoint = circle.center + (velocity * endCapHitInfo.timeOfImpact);
            hitInfo.normal = Normalize(centerPoint - hitInfo.point);
        }
    }
    
    // Ray vs. end-cap circle at line.point2
    {
        Circle end;
        end.center = line.point2;
        end.radius = circle.radius;
        TimeOfImpactInfo endCapHitInfo = SweepPoint(circle.center, velocity, end);
        if (endCapHitInfo.hit && endCapHitInfo.timeOfImpact < hitInfo.timeOfImpact) {
            hitInfo = endCapHitInfo;
            hitInfo.point = line.point2;
            Vector2 centerPoint = circle.center + (velocity * endCapHitInfo.timeOfImpact);
            hitInfo.normal = Normalize(centerPoint - hitInfo.point);
        }
    }
    
    return hitInfo;
}

TimeOfImpactInfo SweepCircle(Circle circle, Vector2 velocity, AABB box) {
    Circle c1;
    c1.center = box.origin;
    c1.radius = circle.radius;
    
    Circle c2 = c1;
    c2.center.y += box.size.y;

    Circle c3 = c1;
    c3.center += box.size;
    
    Circle c4 = c1;
    c4.center.x += box.size.x;
        
    LineSegment l1;
    l1.point1 = c1.center;
    l1.point1.x -= circle.radius;
    l1.point2 = c2.center;
    l1.point2.x -= circle.radius;
    
    LineSegment l2;
    l2.point1 = c1.center;
    l2.point1.y -= circle.radius;
    l2.point2 = c4.center;
    l2.point2.y -= circle.radius;
    
    LineSegment l3;
    l3.point1 = c4.center;
    l3.point1.x += circle.radius;
    l3.point2 = c3.center;
    l3.point2.x += circle.radius;
    
    LineSegment l4;
    l4.point1 = c2.center;
    l4.point1.y += circle.radius;
    l4.point2 = c3.center;
    l4.point2.y += circle.radius;
    
    TimeOfImpactInfo hitInfo = {};
    hitInfo.timeOfImpact = 1.0f;
    
    // test against circles
    
    TimeOfImpactInfo testInfo = SweepPoint(circle.center, velocity, c1); 
    if (hitInfo.timeOfImpact > testInfo.timeOfImpact) {
        hitInfo = testInfo;
    }
    
    testInfo = SweepPoint(circle.center, velocity, c2); 
    if (hitInfo.timeOfImpact > testInfo.timeOfImpact) {
        hitInfo = testInfo;
    }
    
    testInfo = SweepPoint(circle.center, velocity, c3); 
    if (hitInfo.timeOfImpact > testInfo.timeOfImpact) {
        hitInfo = testInfo;
    }
    
    testInfo = SweepPoint(circle.center, velocity, c4); 
    if (hitInfo.timeOfImpact > testInfo.timeOfImpact) {
        hitInfo = testInfo;
    }
    
    // test against lines
    
    testInfo = SweepPoint(circle.center, velocity, l1);
    if (hitInfo.timeOfImpact > testInfo.timeOfImpact) {
        hitInfo = testInfo;
    }
        
    testInfo = SweepPoint(circle.center, velocity, l2);
    if (hitInfo.timeOfImpact > testInfo.timeOfImpact) {
        hitInfo = testInfo;
    }
        
    testInfo = SweepPoint(circle.center, velocity, l3);
    if (hitInfo.timeOfImpact > testInfo.timeOfImpact) {
        hitInfo = testInfo;
    }
        
    testInfo = SweepPoint(circle.center, velocity, l4);
    if (hitInfo.timeOfImpact > testInfo.timeOfImpact) {
        hitInfo = testInfo;
    }
    
    if (hitInfo.hit) {
        Circle dest = circle;
        dest.center += velocity * hitInfo.timeOfImpact;
        hitInfo.point = ClampOnAABB(dest.center, box);
        hitInfo.normal = Normalize(dest.center - hitInfo.point);
    }
    
    return hitInfo;
}

TimeOfImpactInfo SweepCircle(Circle c1, Vector2 velocity, OrientedRectangle obb) {
    TimeOfImpactInfo hitInfo = {};
    hitInfo.timeOfImpact = 1.0f;
    
    // rotate circle and obb coordinate space so we can test Circle vs. AABB
    AABB aabb = {};
    aabb.size = obb.extents * 2;
    
    Circle rCircle = {};
    rCircle.center = RotateRadians(c1.center - obb.center, -obb.rotation) + obb.extents;
    rCircle.radius = c1. radius;
    
    Vector2 rVelocity = RotateRadians(velocity, -obb.rotation);
    
    hitInfo = SweepCircle(rCircle, rVelocity, aabb);
    
    hitInfo.point  = RotateRadians(hitInfo.point - obb.extents, obb.rotation) + obb.center;
    hitInfo.normal = RotateRadians(hitInfo.normal, obb.rotation);
    return hitInfo;
}

TimeOfImpactInfo SweepOBB(OrientedRectangle obb, Vector2 velocity, AABB aabb) {
    TimeOfImpactInfo result = {};
    result.timeOfImpact = 1.0f;

    Vector2 axes[4];
    axes[0] = { 1, 0 };
    axes[1] = { 0, 1 };
    
    float c = cosf(obb.rotation);
    float s = sinf(obb.rotation);
    axes[2] = {  c, s };
    axes[3] = { -s, c };
    
    float entryTime = 0.0f;
    float exitTime  = 1.0f;
    Vector2 hitNormal;
    
    Vector2 obbCorners[4] = {
        axes[2] *  obb.extents.x + axes[3] * obb.extents.y + obb.center,
        axes[2] *  obb.extents.x - axes[3] * obb.extents.y + obb.center,
        axes[2] * -obb.extents.x + axes[3] * obb.extents.y + obb.center,
        axes[2] * -obb.extents.x - axes[3] * obb.extents.y + obb.center,
    };
    
    Vector2 aabbCorners[4] = {
        aabb.origin,
        aabb.origin + Vector2 { aabb.size.x, 0 },
        aabb.origin + Vector2 { 0, aabb.size.y },
        aabb.origin + aabb.size
    };
    
    for (int i = 0; i < countOf(axes); i++) {
        Vector2 axis = axes[i]; 
    
        float obbMin, obbMax;
        obbMin = DotProduct(obbCorners[0], axis);
        obbMax = obbMin;
        
        for (int j = 1; j < countOf(obbCorners); j++) {
            float proj = DotProduct(obbCorners[j], axis);
            obbMin = Min(proj, obbMin);
            obbMax = Max(proj, obbMax);
        }
        
        float aabbMin, aabbMax;
        aabbMin = DotProduct(aabbCorners[0], axis);
        aabbMax = aabbMin;
        
        for (int j = 0; j < countOf(aabbCorners); j++) {
            float proj = DotProduct(aabbCorners[j], axis);
            aabbMin = Min(proj, aabbMin);
            aabbMax = Max(proj, aabbMax);
        }
                        
        float v = DotProduct(velocity, axis);
        
        if (v == 0) {
            if (obbMax < aabbMin || obbMin > aabbMax) {
                // separated and no movement
                return result;
            }
            continue;
        }
        
        float t0 = (aabbMin - obbMax) / v;
        float t1 = (aabbMax - obbMin) / v;
        if (t0 > t1) {
            SWAP(t0, t1);
        }
        
        t0 = Max(t0, 0.0f);
        t1 = Min(t1, 1.0f);
        if (t0 > t1) {
            return result;
        }
        
        if (t0 > entryTime) {
            entryTime = t0;
            hitNormal = axis * -Sign(v);
        }
        
        exitTime = Min(exitTime, t1);
        if (entryTime > exitTime) {
            return result;
        }
    }
    
    result.hit = true;
    result.timeOfImpact = entryTime;
    result.normal = hitNormal;
    result.point = obb.center + velocity * entryTime;
    
    // SPEED(roger): We don't really need to calculate a hit point for TimeOfImpact calculations.
    // There are physics in the game that are only concerned with moving a thing forward and making sure
    // it slides or bounces off that surface which means you only really need hitNormal and timeOfImpact.
    // However, we could move this to a separate function that can be used after detecting an impact to 
    // give us the closest hit point. We can do this for all time of impact functions. Essentially remove 
    // point from the TimeOfImpactInfo struct. 
    
    Vector2 displacement = velocity * entryTime;
    obbCorners[0] += displacement;
    obbCorners[1] += displacement;
    obbCorners[2] += displacement;
    obbCorners[3] += displacement;
    
    // Is the OBB axis aligned?
    if (IsAxisAligned(axes[2])) {

        Vector2 obbMin = obbCorners[3];
        Vector2 obbMax = obbCorners[0];

        Range xRange;
        xRange.min = Max(aabb.origin.x, obbMin.x);
        xRange.max = Min(aabb.origin.x + aabb.size.x, obbMax.x);
        result.point.x = (xRange.min + xRange.max) * 0.5f;
        
        Range yRange;
        yRange.min = Max(aabb.origin.y, obbMin.y);
        yRange.max = Min(aabb.origin.y + aabb.size.y, obbMax.y);
        result.point.y = (yRange.min + yRange.max) * 0.5f;
    
    } else if (IsAxisAligned(hitNormal)) {
        float min = DotProduct(obbCorners[0], hitNormal);
        result.point = obbCorners[0];
        
        for (int j = 1; j < 4; j++) {
            float proj = DotProduct(obbCorners[j], hitNormal);
            if (proj < min) {
                min = proj;
                result.point = obbCorners[j];
            }
        }
    } else {
        float max = DotProduct(aabbCorners[0], hitNormal);
        result.point = aabbCorners[0];
        
        for (int j = 1; j < 4; j++) {
            float proj = DotProduct(aabbCorners[j], hitNormal);
            if (proj > max) {
                max = proj;
                result.point = aabbCorners[j];
            }
        }
    }
    
    return result;
}

TimeOfImpactInfo SweepOBB(OrientedRectangle r1, Vector2 velocity, OrientedRectangle r2) {
    AABB aabb = {};
    aabb.origin = r2.extents * -1;
    aabb.size = r2.extents * 2;
    
    r1.rotation -= r2.rotation;
    r1.center = RotateRadians(r1.center - r2.center, -r2.rotation);
    
    Vector2 rVelocity = RotateRadians(velocity, -r2.rotation);
    
    TimeOfImpactInfo hitInfo = SweepOBB(r1, rVelocity, aabb);
    
    hitInfo.point = RotateRadians(hitInfo.point, r2.rotation) + r2.center;
    hitInfo.normal = RotateRadians(hitInfo.normal, r2.rotation);
    
    return hitInfo;
}

TimeOfImpactInfo SweepOBB(OrientedRectangle obb, Vector2 velocity, LineSegment line) {
    AABB aabb = {};
    aabb.size = obb.extents * 2;
    
    LineSegment rLine;
    rLine.point1 = RotateRadians(line.point1 - obb.center, -obb.rotation) + obb.extents;
    rLine.point2 = RotateRadians(line.point2 - obb.center, -obb.rotation) + obb.extents;

    Vector2 rVelocity = RotateRadians(velocity, -obb.rotation);

    TimeOfImpactInfo hitInfo = SweepAABB(aabb, rVelocity, rLine);
    
    hitInfo.point = RotateRadians(hitInfo.point - obb.extents, obb.rotation) + obb.center;
    hitInfo.normal = RotateRadians(hitInfo.normal, obb.rotation);

    return hitInfo;
}

Rect ScaleRect(Rect rect, Vector2 scale) {
    rect.hw *= scale.x;
    rect.hh *= scale.y;
    return rect;
}

bool IsPointInsideRect(Vector2 p, Rect rect) {
    float bottomLeftX = rect.x - rect.hw;
    float bottomLeftY = rect.y - rect.hh;
    float topRightX = rect.x + rect.hw;
    float topRightY = rect.y + rect.hh;

    return p.x >= bottomLeftX && p.x <= topRightX &&
        p.y >= bottomLeftY && p.y <= topRightY;
}

Rect EncapsulateRects(const Rect& rect1, const Rect& rect2) {
    // Find the min and max x and y coordinates for the encapsulating rectangle
    float minX = fmin(rect1.x - rect1.hw, rect2.x - rect2.hw);
    float maxX = fmax(rect1.x + rect1.hw, rect2.x + rect2.hw);
    float minY = fmin(rect1.y - rect1.hh, rect2.y - rect2.hh);
    float maxY = fmax(rect1.y + rect1.hh, rect2.y + rect2.hh);

    // Create the encapsulating rectangle
    Rect encapsulatingRect;
    encapsulatingRect.x = (minX + maxX) / 2;  // Center x
    encapsulatingRect.y = (minY + maxY) / 2;  // Center y
    encapsulatingRect.hw = (maxX - minX) / 2; // Half width
    encapsulatingRect.hh = (maxY - minY) / 2; // Half height

    return encapsulatingRect;
}

void EncapsulateAABB(AABB* aabb, Vector2 point) {
    Vector2 min = aabb->origin;
    Vector2 max = {aabb->origin.x + aabb->size.x, aabb->origin.y + aabb->size.y};

    if (point.x < min.x) min.x = point.x;
    if (point.y < min.y) min.y = point.y;
    if (point.x > max.x) max.x = point.x;
    if (point.y > max.y) max.y = point.y;

    aabb->origin = min;
    aabb->size.x = max.x - min.x;
    aabb->size.y = max.y - min.y;
}

float AreaOfTriangle(Vector2 a, Vector2 b, Vector2 c) {
    Vector2 ab = a - b;
    Vector2 bc = b - c;
    float area = CrossProduct(ab, bc);
    return area * 0.5f;
}

// Convert HSV to RGB
Vector4 HSVtoRGB(float h, float s, float v, float a) {
    if (s == 0.0f) {
        // gray
        return {v, v, v, a};
    }

    h = fmod(h, 1.0f) / (60.0f / 360.0f);
    int   i = (int)h;
    float f = h - (float)i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));

    switch (i) {
        case 0: { return {v, t, p, a}; } break;
        case 1: { return {q, v, p, a}; } break;
        case 2: { return {p, v, t, a}; } break;
        case 3: { return {p, q, v, a}; } break;
        case 4: { return {t, p, v, a}; } break;
        case 5: default: { return {v, p, q, a}; } break;
    }
}

Vector4 HSVtoRGB(Vector4 hsv) {
    return HSVtoRGB(hsv.x, hsv.y, hsv.z, hsv.a);  
}

Vector4 RGBtoHSV(float r, float g, float b, float a) {
    float K = 0.f;
    if (g < b) {
        SWAP(g, b);
        K = -1.f;
    }
    
    if (r < g) {
        SWAP(r, g);
        K = -2.f / 6.f - K;
    }

    float chroma = r - (g < b ? g : b);
    float h = Abs(K + (g - b) / (6.0f * chroma + 1e-20f));
    float s = chroma / (r + 1e-20f);
    float v = r;
    
    return { h, s, v, a };
}

Vector4 RGBtoHSV(Vector4 rgb) {
    return RGBtoHSV(rgb.r, rgb.b, rgb.b, rgb.a);
}

// Generate a pastel color palette
Vector4 GetPastelColor(int index) {
    float hue = (index * 0.618033988749895f); // Golden ratio conjugate
    hue = fmod(hue, 1.0f); // Ensure hue is in [0, 1]
    return HSVtoRGB(hue, 0.5f, 0.9f, 1.0f); // Pastel colors with lower saturation and high value
}

float PingPong(float time, float min, float max, float speed) {
    float range = max - min;
    float doubledRange = range * 2.0f;
    if (range <= 0.0f) {
        return min; // Handle invalid range
    }
    // Calculate the value based on time and speed
    float value = time * speed;
    // Normalize value within the doubled range
    float normalizedValue = fmod(value - min, doubledRange);
    // If normalized value is negative, bring it into the positive range
    if (normalizedValue < 0.0f) {
        normalizedValue += doubledRange;
    }
    // Reflect back if the value is in the second half of the range
    if (normalizedValue > range) {
        return max - (normalizedValue - range);
    }
    return min + normalizedValue;
}

//Curves
SERIALIZE() struct CurveKeyframe {
    WRITE() float time;
    WRITE() float value;
    WRITE() float inSlope;  //tangent coming into the keyframe
    WRITE() float outSlope; //tangent leaving the keyframe
};

SERIALIZE() struct Curve {
    WRITE() u32 keyframeCount;
    WRITE_ARRAY(keyframeCount) CurveKeyframe* keyframes;
};

void ComputeClampedAutoTangents(Curve* curve) {
    CurveKeyframe* kf = curve->keyframes;
    u32 count = curve->keyframeCount;
    for (u32 i = 0; i < count; ++i) {
        float tangent = 0.0f;

        // Endpoints: use difference with neighbor
        if (i == 0) {
            tangent = (kf[1].value - kf[0].value) / (kf[1].time - kf[0].time);
        } else if (i == count - 1) {
            tangent = (kf[count-1].value - kf[count-2].value) / (kf[count-1].time - kf[count-2].time);
        } else {
            float dtPrev = kf[i].time   - kf[i-1].time;
            float dtNext = kf[i+1].time - kf[i].time;
            float dvPrev = kf[i].value  - kf[i-1].value;
            float dvNext = kf[i+1].value- kf[i].value;
            tangent = (dvPrev/dtPrev + dvNext/dtNext) * 0.5f;

            // Clamp at extremum (flat tangent)
            if ((kf[i].value > kf[i-1].value && kf[i].value > kf[i+1].value) ||
                (kf[i].value < kf[i-1].value && kf[i].value < kf[i+1].value)) {
                tangent = 0.0f;
            }
        }

        kf[i].inSlope  = tangent;
        kf[i].outSlope = tangent;
    }
}

float HermiteInterpolation(float t, CurveKeyframe f0, CurveKeyframe f1) {
    float h00 = 2 * t * t * t - 3 * t * t + 1;
    float h10 = t * t * t - 2 * t * t + t;
    float h01 = -2 * t * t * t + 3 * t * t;
    float h11 = t * t * t - t * t;
    float dt = f1.time - f0.time;
    return h00 * f0.value + h10 * dt * f0.outSlope + h01 * f1.value + h11 * dt * f1.inSlope;
}

float EvaluateCurve(Curve curve, float t) {
    CurveKeyframe* keyframes = curve.keyframes;
    u32 keyframeCount = curve.keyframeCount;
    
    if (t <= keyframes[0].time) { 
        return keyframes[0].value; 
    }
    
    if (t >= keyframes[keyframeCount - 1].time) {
        return keyframes[keyframeCount-1].value;
    }
    
    for (u32 i = 0; i < keyframeCount-1; i++) {
        if (t <= keyframes[i+1].time) {
            float localT = (t - keyframes[i].time) / (keyframes[i+1].time - keyframes[i].time);
            return HermiteInterpolation(localT, keyframes[i], keyframes[i+1]);
        }
    }
    
    ASSERT_ERROR(false, "This should never happen.");
    return 0.0f;
}

Vector2 QuadraticBezierLerp(Vector2 a, Vector2 b, Vector2 control, float t) {
    float inverseT = 1 - t;
    return (a * (inverseT*inverseT)) + (control * 2 * inverseT * t) + (b * (t*t));
}

Vector2 QuadraticBezierTangent(Vector2 a, Vector2 b, Vector2 control, float t) {
    float inverseT = 1 - t;
    return ((control - a) * 2 * inverseT) + ((b - control) * 2 * t);
}

//GJK Algorithm: For collision

enum GJKSupportType {
    GJKSupportType_LineSegment,
    GJKSupportType_Circle,
    GJKSupportType_AABB,
    GJKSupportType_Polygon,
};

struct GJKSupportData {
    GJKSupportType type;
    bool swept;
    Vector2 sweptDirection;
    union {
        AABB aabb;
        LineSegment lineSegment;
        Circle circle;
        struct {
            Vector2* vertices;
            u32 count;
        } polygon;
    };
};

Vector2 PickSupportPoint(GJKSupportData* shape, Vector2 d) {
    Vector2 support;
    switch(shape->type) {
        case GJKSupportType_LineSegment: {
            float dot1 = DotProduct(shape->lineSegment.point1, d);
            float dot2 = DotProduct(shape->lineSegment.point2, d);
            support = (dot1 > dot2) ? shape->lineSegment.point1 : shape->lineSegment.point2;
        } break;
        case GJKSupportType_Circle: {
            Vector2 normalizedDir = Normalize(d);
            support = shape->circle.center + normalizedDir * shape->circle.radius;
        } break;
        case GJKSupportType_AABB: {
            support.x = (d.x > 0) ? shape->aabb.origin.x + shape->aabb.size.x : shape->aabb.origin.x;
            support.y = (d.y > 0) ? shape->aabb.origin.y + shape->aabb.size.y : shape->aabb.origin.y;
        } break;
        case GJKSupportType_Polygon: {
            float maxProduct = DotProduct(d, shape->polygon.vertices[0]);
            u32 index = 0;
            for (u32 i = 1; i < shape->polygon.count; i++) {
                float product = DotProduct(d, shape->polygon.vertices[i]);
                if (product > maxProduct) {
                    maxProduct = product;
                    index = i;
                }
            }
            support = shape->polygon.vertices[index];
        } break;
        default: {
            NOT_IMPLEMENTED();
        }
    }
    if (shape->swept) {
        support += shape->sweptDirection;
    }
    return support;
}

struct GJKSupportPoint {
    Vector2 point; //the point on the minkowski sum
    Vector2 s1; //From shape 1
    Vector2 s2; //From shape 2
};

GJKSupportPoint GJK_Support(GJKSupportData* shape1, GJKSupportData* shape2, Vector2 d) {
    GJKSupportPoint support;
    support.s1 = PickSupportPoint(shape1, d);
    support.s2 = PickSupportPoint(shape2, d * -1);
    support.point = support.s1 - support.s2;
    return support;
}

int GJK(GJKSupportData* shape1, GJKSupportData* shape2) {
    u32 simplexCount = 0;
    GJKSupportPoint a, b, c, simplex[3];
    Vector2 ao, ab, ac, abperp, acperp;
    Vector2 d = {1, 0}; //arbitrary initial direction.

    //set the first support as initial point of the new simplex
    a = simplex[0] = GJK_Support(shape1, shape2, d);
    d = a.point * -1;

    while (true) {
        a = simplex[++simplexCount] = GJK_Support(shape1, shape2, d);
        if (DotProduct(a.point, d) <= 0) {
            return 0; //no collision
        }
        ao = a.point * -1;
        switch (simplexCount) {
            //Line Segment
            case 2: {
                b = simplex[0];
                ab = b.point - a.point;
                d = TripleProduct(ab, ao, ab); //Normal to AB towards Origin
                if (SqrMagnitude(d) == 0) {
                    d = Perpendicular(ab);
                }
                continue; //skip to next iteration
            } break;
            //Triangle
            case 3: {
                b = simplex[1];
                c = simplex[0];
                ab = b.point - a.point;
                ac = c.point - a.point;
                abperp = TripleProduct(ac, ab, ab);
                if (DotProduct(abperp, ao) > 0) {
                    simplex[0] = simplex[1];
                    d = abperp;
                } else {
                    acperp = TripleProduct(ab, ac, ac);
                    if (DotProduct(acperp, ao) > 0) {
                        d = acperp;
                    } else {
                        return 1;
                    }
                }
                simplex[1] = simplex[2];
                --simplexCount;
            }
        }
    }
    return 0;
}

struct GJKDistanceResult {
    GJKSupportPoint witnessA;
    GJKSupportPoint witnessB;
    Vector2 closestDistance;
};

//NOTE(roger): GJK only reports closest distance (nonIntersectionResult) if there is no intersection.
bool GJK_Distance(GJKSupportData* shape1, GJKSupportData* shape2, GJKDistanceResult* nonIntersectionResult) {
    u32 simplexCount = 0;
    GJKSupportPoint a, b, c, simplex[3];
    Vector2 ao, ab, abperp, acperp;
    Vector2 d = {1, 0}; //arbitrary initial direction.

    //set the first support as initial point of the new simplex
    a = simplex[0] = GJK_Support(shape1, shape2, d);
    d = d * -1;

    while (true) {
        b = simplex[simplexCount];
        a = simplex[++simplexCount] = GJK_Support(shape1, shape2, d);

        //Duplicate Points
        if (simplex[0].point == a.point || b.point == a.point) {
            //Duplicate point indicates that we found the most extreme edge.
            //so we prepare the result and exit.

            nonIntersectionResult->witnessA = simplex[1];
            nonIntersectionResult->witnessB = simplex[0];
            nonIntersectionResult->closestDistance = ClosestPointOnLineToOrigin(simplex[1].point, simplex[0].point);
            return false;
        }

        ao = a.point * -1;
        ab = b.point - a.point;
        //Distance along AB
        float t = DotProduct(ao, ab) / DotProduct(ab, ab);
        //Is Origin?
        if (t >= 0 && t <= 1.0f) {
            Vector2 p = a.point + (ab * t);
            if (Abs(p.x) < EPSILON && Abs(p.y) < EPSILON) {
                return true;
            }
        }

        switch (simplexCount + 1) {
            //Line Segment
            case 2: {
                //Normal to AB towards Origin
                d = TripleProduct(ab, ao, ab);
                if (SqrMagnitude(d) == 0) {
                    d = Perpendicular(ab);
                }
                continue;
            } break;
            //Triangle
            case 3: {
                c = simplex[0];

                bool isOriginInsideTriangle = false;

                // Calculate vectors
                Vector2 ac = c.point - a.point;

                // Compute dot products
                float dot00 = DotProduct(ac, ac);
                float dot01 = DotProduct(ac, ab);
                float dot02 = DotProduct(ac, ao);
                float dot11 = DotProduct(ab, ab);
                float dot12 = DotProduct(ab, ao);

                // Compute barycentric coordinates
                float denom = dot00 * dot11 - dot01 * dot01;
                //Make sure triangle simplex is not degenerate.
                if (denom != 0.0f) {
                    float invDenom = 1.0f / denom;
                    float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
                    float v = (dot00 * dot12 - dot01 * dot02) * invDenom;
                    isOriginInsideTriangle = (u >= 0) && (v >= 0) && (u + v <= 1);
                }

                // Check if point is in triangle
                if (isOriginInsideTriangle) {
                    return true;
                } else {
                    Vector2 p1 = ClosestPointOnLineToOrigin(a.point, c.point);
                    Vector2 p2 = ClosestPointOnLineToOrigin(b.point, c.point);
                    if (SqrMagnitude(p1) < SqrMagnitude(p2)) {
                        b = c;
                        simplex[1] = simplex[2];
                        simplexCount--;
                        d = p1 * -1;
                    } else {
                        a = c;
                        simplex[0] = simplex[2];
                        simplexCount--;
                        d = p2 * -1;
                    }
                }
            }
        }
    }
    return false;
}

int SutherlandHodgmanClip(AABB clip, OrientedRectangle obb, Vector2* output) {
    const u32 CLIP_VERTEX_COUNT = 4;
    Vector2 clipVertices[CLIP_VERTEX_COUNT];
    clipVertices[0] = clip.origin;
    clipVertices[1] = clip.origin; clipVertices[1].y += clip.size.y;
    clipVertices[2] = clip.origin + clip.size;
    clipVertices[3] = clip.origin; clipVertices[3].x += clip.size.x;

    const u32 MAX_VERTICES = 16;
    u32 inputCount = 4;
    Vector2 inputVertices[MAX_VERTICES];
    inputVertices[0] = OrientedRectangleCorner(obb, 0);
    inputVertices[1] = OrientedRectangleCorner(obb, 1);
    inputVertices[2] = OrientedRectangleCorner(obb, 2);
    inputVertices[3] = OrientedRectangleCorner(obb, 3);

    for (u32 i = 0; i < CLIP_VERTEX_COUNT; i++) {
        Vector2 edgeStart = clipVertices[i];
        Vector2 edgeEnd   = clipVertices[(i + 1) % CLIP_VERTEX_COUNT];

        Vector2 edgeDirection = edgeEnd - edgeStart;

        u32 tempCount = 0;
        Vector2 tempVertices[MAX_VERTICES];

        for (u32 j = 0; j < inputCount; j++) {
            Vector2 previous = inputVertices[j];
            Vector2 current  = inputVertices[(j + 1) % inputCount];

            Vector2 previousDirection = previous - edgeStart;
            Vector2 currentDirection  = current  - edgeStart;

            bool isPreviousInside = CrossProduct(edgeDirection, previousDirection) <= 0;
            bool isCurrentInside  = CrossProduct(edgeDirection, currentDirection)  <= 0;

            if (isCurrentInside) {
                if (!isPreviousInside) {
                    Line edge      = { edgeStart, edgeDirection      };
                    Line inputEdge = { previous,  current - previous };
                    tempVertices[tempCount++] = LineIntersection(edge, inputEdge);
                }
                tempVertices[tempCount++] = current;
            } else if (isPreviousInside) {
                Line edge      = { edgeStart, edgeDirection      };
                Line inputEdge = { previous,  current - previous };
                tempVertices[tempCount++] = LineIntersection(edge, inputEdge);
            }
        }

        inputCount = tempCount;
        CopyArray(inputVertices, tempVertices, tempCount);
    }

    CopyArray(output, inputVertices, inputCount);
    return inputCount;
}

// #include "assert.h"
//
// void TestGameMath() {
//     //Vector2
//     {
//         Vector2 a = { 1, 2 };
//         Vector2 b = { 3, 5 };
//         Vector2 c = a + b;

//         Vector2 assertValue = { 4, 7 };
//         assert(c == assertValue);

//         c = b - a;
//         assertValue = { 2, 3 };
//         assert(c == assertValue);

//         Vector2 d = Negate(c);
//         assertValue = { -2, -3 };
//         assert(d == assertValue);

//         d = d * 5;
//         assertValue = { -10, -15 };
//         assert(d == assertValue);

//         d = d / 2;
//         assertValue = { -5, -7.5 };
//         assert(d == assertValue);

//         assert(d[0] == -5);

//         d *= 3;
//         assertValue = { -15, -22.5 };
//         assert(d == assertValue);

//         d /= 2;
//         assertValue = { -7.5, -11.25 };
//         assert(d == assertValue);

//         d = { 2, 2 };
//         float aDist = Magnitude(d);
//         assert(FloatsEqual(aDist, 2.82842712f));

//         d = { 5, 0 };
//         d = Normalize(d);
//         assertValue = { 1, 0 };
//         assert(d == assertValue);

//         d = {1, 0};
//         d = Rotate(d, 90);
//         assertValue = { 0, 1 };
//         assert(d == assertValue);

//         Rotate(d, 360);
//         assert(d == assertValue);

//         a = {  8, 2 };
//         b = { -2, 8 };
//         c = { -5, 5 };

//         assert(FloatsEqual(0, DotProduct(a, b)));
//         assert(0 > DotProduct(a, c));
//         assert(0 < DotProduct(b, c));

//         float angle = CalculateEnclosedAngle(a, b);
//         assert(FloatsEqual(90, angle));
//     }

//     //AABB-AABB collision
//     {
//         AABB a = {{1, 1}, {4, 4}};
//         AABB b = {{2, 2}, {5, 5}};
//         AABB c = {{6, 4}, {4, 2}};

//         assert(AABB_Collide(a, b) == true);
//         assert(AABB_Collide(b, c) == true);
//         assert(AABB_Collide(a, c) == false);
//     }

//     //Circle-Circle collision
//     {
//         Circle a = {{ 4, 4}, 2};
//         Circle b = {{ 7, 4}, 2};
//         Circle c = {{10, 4}, 2};

//         assert(Circle_Collide(a, b) == true);
//         assert(Circle_Collide(b, c) == true);
//         assert(Circle_Collide(a, c) == false);
//     }

//     //Line-Line collision
//     {
//         Vector2 a = {3, 5};
//         Vector2 b = {3, 2};
//         Vector2 c = {8, 4};

//         Vector2 down = {5, -1};
//         Vector2 up   = {5,  2};

//         Line l1 = {a, down};
//         Line l2 = {a, up};
//         Line l3 = {b, up};
//         Line l4 = {c, down};

//         assert(Line_Collide(l1, l2) == true);
//         assert(Line_Collide(l1, l3) == true);
//         assert(Line_Collide(l2, l3) == false);
//         assert(Line_Collide(l1, l4) == true);
//     }

//     //Segment-Segment collision
//     {
//         Vector2 a = { 3, 4};
//         Vector2 b = {11, 1};
//         Vector2 c = { 8, 4};
//         Vector2 d = {11, 7};

//         LineSegment s1 = {a, b};
//         LineSegment s2 = {c, d};

//         assert(LineSegment_Collide(s1, s2) == false);
//     }

//     //OrientedRectangle-OrientedRectangle collision
//     {
//         OrientedRectangle a = {{3,  5}, {1, 3},  15};
//         OrientedRectangle b = {{10, 5}, {2, 2}, -15};
//         OrientedRectangle c = {{5, 5}, {3, 2}, 30};
//         OrientedRectangle d = {{7, 7}, {2, 4}, -30};

//         assert(OrientedRectangle_Collide(a, b) == false);
//         assert(OrientedRectangle_Collide(c, d) == true);
//     }

//     //Circle-Point collision
//     {
//         Circle c = {{6, 4}, 3};
//         Vector2 p1 = {8, 3};
//         Vector2 p2 = {11, 7};
//         assert(IsPointInsideCircle(c, p1) == true);
//         assert(IsPointInsideCircle(c, p2) == false);
//     }

//     //Circle-Line collision
//     {
//         Circle c = {{6, 3}, 2};
//         Line l = {{4, 7}, {5, -1}};
//         assert(CircleLine_Collide(c, l) == false);
//     }

//     //Circle-LineSegment collision
//     {
//         Circle c = {{4, 4}, 3};
//         LineSegment s = {{8, 3}, {13, 6}};
//         assert(CircleLineSegment_Collide(c, s) == false);
//     }

//     //Circle-AABB collision
//     {
//         AABB r = {{3, 2}, {6, 4}};
//         Circle c1 = {{5, 4}, 1};
//         Circle c2 = {{7, 8}, 1};
//         assert(CircleAABB_Collide(c1, r) == true);
//         assert(CircleAABB_Collide(c2, r) == false);
//     }

//     //Circle-OrientedRectangle collision
//     {
//         OrientedRectangle r = {{5, 4}, {3, 2}, 30};
//         Circle c = {{5, 7}, 2};
//         assert(CircleOrientedRectangle_Collide(c, r) == true);
//     }

//     //Line-AABB collision
//     {
//         Line line = {{6, 8}, {2, -3}};
//         AABB aabb = {{3, 2}, {6,  4}};
//         assert(LineAABB_Collide(line, aabb) == true);
//     }
// }

#endif