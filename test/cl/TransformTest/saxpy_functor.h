struct SaxpyFunctor
{
	float _a;
	SaxpyFunctor(float a) : _a(a) {};
	float operator() (const float &xx, const float &yy)
	{
		return _a * xx + yy;
	};
};