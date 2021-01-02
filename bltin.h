struct Bltin_name {
	char *s;
	double (*f)(Arg *);
};

extern struct Bltin_name bltins[];

double Pi(Arg *);
double E(Arg *);
double Gamma(Arg *);
double Deg(Arg *);
double Phi(Arg *);
double Abs(Arg *);
double Atan(Arg *);
double Atan2(Arg *);
double Cos(Arg *);
double Exp(Arg *);
double Integer(Arg *);
double Log(Arg *);
double Log10(Arg *);
double Sin(Arg *);
double Sqrt(Arg *);
