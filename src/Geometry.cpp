#include <string>

template <typename T>
class Vec3
{
public:
	Vec3() :
		x { 0 },
		y { 0 },
		z { 0 }
	{}
	Vec3(T v) :
		x { v },
		y { v },
		z { v }
	{}
	Vec3(T xx, T yy, T zz) :
		x { xx },
		y { yy },
		z { zz }
	{}
	Vec3 operator+(const Vec3& v) const
	{
		return Vec3(x + v.x, y + v.y, z + v.z);
	}
	void operator+=(const Vec3& v)
	{
		x += v.x;
		y += v.y;
		z += v.z;
	}
	Vec3 operator-(const Vec3& v) const
	{
		return Vec3(x - v.x, y - v.y, z - v.z);
	}
	void operator-=(const Vec3& v)
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
	}
	Vec3 operator*(const T& f) const
	{
		return Vec3(x * f, y * f, z * f);
	}
	void operator*=(const T& f)
	{
		x *= f;
		y *= f;
		z *= f;
	}
	Vec3 operator/(const T& f) const
	{
		return Vec3(x / f, y / f, z / f);
	}
	void operator/=(const T& f)
	{
		x /= f;
		y /= f;
		z /= f;
	}
	const T& operator[](uint8_t i) const
	{
		return (&x)[i];
	}
	T& operator[](uint8_t i)
	{
		return (&x)[i];
	}

	T norm() const
	{
		return x * x + y * y + z * z;
	}
	T length() const
	{
		return sqrt(norm());
	}

	void normalize()
	{
		T f = 1 / length();
		x *= f, y *= f, z *= f;
	}

	template <typename S>
	float dotProduct(const Vec3<S>& b) const
	{
		return x * b.x + y * b.y + z * b.z;
	}

	template <typename S>
	Vec3<S> crossProduct(const Vec3<S>& b) const
	{
		return Vec3<S>(
			y * b.z - z * b.y,
			z * b.x - x * b.z,
			x * b.y - y * b.x);
	}

	// // Randomizes the vector 0 - 1
	// void Randomize()
	// {
	// 	x = rand() % 1000.0)
	// }

	std::string ToString() const
	{
		return "Vec3(x=" + std::to_string(x) + ", y=" + std::to_string(y) + ", z=" + std::to_string(z) + ")";
	}

	T x, y, z;
};

typedef Vec3<float> Vec3f;
typedef Vec3<int> Vec3i;

template <typename T>
class Matrix44
{
public:
	T x[4][4] = { { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } };

	Matrix44()
	{
		x[0][0] = 1;
		x[1][1] = 1;
		x[2][2] = 1;
		x[3][3] = 1;
	}

	Matrix44(T a, T b, T c, T d, T e, T f, T g, T h,
		T i, T j, T k, T l, T m, T n, T o, T p)
	{
		x[0][0] = a;
		x[0][1] = b;
		x[0][2] = c;
		x[0][3] = d;
		x[1][0] = e;
		x[1][1] = f;
		x[1][2] = g;
		x[1][3] = h;
		x[2][0] = i;
		x[2][1] = j;
		x[2][2] = k;
		x[2][3] = l;
		x[3][0] = m;
		x[3][1] = n;
		x[3][2] = o;
		x[3][3] = p;
	}

	template <typename S>
	void multVecMatrix(const Vec3<S>& src, Vec3<S>& dst) const
	{
		S a, b, c, w;

		a = src[0] * x[0][0] + src[1] * x[1][0] + src[2] * x[2][0] + x[3][0];
		b = src[0] * x[0][1] + src[1] * x[1][1] + src[2] * x[2][1] + x[3][1];
		c = src[0] * x[0][2] + src[1] * x[1][2] + src[2] * x[2][2] + x[3][2];
		w = src[0] * x[0][3] + src[1] * x[1][3] + src[2] * x[2][3] + x[3][3];

		dst.x = a / w;
		dst.y = b / w;
		dst.z = c / w;
	}

	template <typename S>
	void multDirMatrix(const Vec3<S>& src, Vec3<S>& dst) const
	{
		S a, b, c;

		a = src[0] * x[0][0] + src[1] * x[1][0] + src[2] * x[2][0];
		b = src[0] * x[0][1] + src[1] * x[1][1] + src[2] * x[2][1];
		c = src[0] * x[0][2] + src[1] * x[1][2] + src[2] * x[2][2];

		dst.x = a;
		dst.y = b;
		dst.z = c;
	}
};

typedef Matrix44<float> Matrix44f;
typedef Matrix44<int> Matrix44i;
