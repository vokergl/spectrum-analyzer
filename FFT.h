//https://ru.wikibooks.org/wiki/%D0%A0%D0%B5%D0%B0%D0%BB%D0%B8%D0%B7%D0%B0%D1%86%D0%B8%D0%B8_%D0%B0%D0%BB%D0%B3%D0%BE%D1%80%D0%B8%D1%82%D0%BC%D0%BE%D0%B2/%D0%91%D1%8B%D1%81%D1%82%D1%80%D0%BE%D0%B5_%D0%BF%D1%80%D0%B5%D0%BE%D0%B1%D1%80%D0%B0%D0%B7%D0%BE%D0%B2%D0%B0%D0%BD%D0%B8%D0%B5_%D0%A4%D1%83%D1%80%D1%8C%D0%B5

#define  NUMBER_IS_2_POW_K(x)   ((!((x)&((x)-1)))&&((x)>1))
#define  FT_DIRECT        -1    // Direct transform.
#define  FT_INVERSE        1    // Inverse transform.

int ln(int n);

char FFT(float *Rdat, float *Idat, int N, int Ft_Flag)
{
	int LogN = ln(N);

	if((Rdat == 0) || (Idat == 0))
	{
		return 0;
	}

	if((N > 16384) || (N < 1))
	{
		return 0;
	}

	if(!NUMBER_IS_2_POW_K(N))
	{
		return 0;
	}

	if((LogN < 2) || (LogN > 14))
	{
		return 0;
	}

	if((Ft_Flag != FT_DIRECT) && (Ft_Flag != FT_INVERSE))
	{
		return 0;
	} 

	register int i, j, n, k, io, ie, in, nn;
	float ru, iu, rtp, itp, rtq, itq, rw, iw, sr;

	static const float Rcoef[14] =
	{  
		-1.0000000000000000F,  0.0000000000000000F,  0.7071067811865475F,
		0.9238795325112867F,  0.9807852804032304F,  0.9951847266721969F,
		0.9987954562051724F,  0.9996988186962042F,  0.9999247018391445F,
		0.9999811752826011F,  0.9999952938095761F,  0.9999988234517018F,
		0.9999997058628822F,  0.9999999264657178F
	};

	static const float Icoef[14] =
	{   
		0.0000000000000000F, -1.0000000000000000F, -0.7071067811865474F,
		-0.3826834323650897F, -0.1950903220161282F, -0.0980171403295606F,
		-0.0490676743274180F, -0.0245412285229122F, -0.0122715382857199F,
		-0.0061358846491544F, -0.0030679567629659F, -0.0015339801862847F,
		-0.0007669903187427F, -0.0003834951875714F
	};

	nn = N >> 1;
	ie = N;

	for(n=1; n<=LogN; n++)
	{
		rw = Rcoef[LogN - n];
		iw = Icoef[LogN - n];

		if(Ft_Flag == FT_INVERSE) iw = -iw;

		in = ie >> 1;

		ru = 1.0F;
		iu = 0.0F;

		for(j=0; j<in; j++)
		{
			for(i=j; i<N; i+=ie)
			{
				io       = i + in;
				rtp      = Rdat[i]  + Rdat[io];
				itp      = Idat[i]  + Idat[io];
				rtq      = Rdat[i]  - Rdat[io];
				itq      = Idat[i]  - Idat[io];
				Rdat[io] = rtq * ru - itq * iu;
				Idat[io] = itq * ru + rtq * iu;
				Rdat[i]  = rtp;
				Idat[i]  = itp;
			}

			sr = ru;
			ru = ru * rw - iu * iw;
			iu = iu * rw + sr * iw;
		}

		ie >>= 1;
	}

	for(j=i=1; i<N; i++)
	{
		if(i < j)
		{
			io       = i - 1;
			in       = j - 1;
			rtp      = Rdat[in];
			itp      = Idat[in];
			Rdat[in] = Rdat[io];
			Idat[in] = Idat[io];
			Rdat[io] = rtp;
			Idat[io] = itp;
		}

		k = nn;

		while(k < j)
		{
			j   = j - k;
			k >>= 1;
		}

		j = j + k;
	}

	if(Ft_Flag == FT_DIRECT) return 1;

	rw = 1.0F / N;

	for(i=0; i<N; i++)
	{
		Rdat[i] *= rw;
		Idat[i] *= rw;
	}

	return 1;
}

int ln(int n)
{
	if(n < 4 || n > 16384)
	{
		return 0;
	}

	switch(n)
	{
	case 4: return 2;
	case 8: return 3;
	case 16: return 4;
	case 32: return 5;
	case 64: return 6;
	case 128: return 7;
	case 256: return 8;
	case 512: return 9;
	case 1024: return 10;
	case 2048: return 11;
	case 4096: return 12;
	case 8192: return 13;
	case 16384: return 14;
	}

	return 0;
}

float _sqrt(float x)
{
	if(x <= 0)
	{
		return 0;
	}

	float a = 3.162278;

	if(x >= 500.0)
	{
		a = 31.62278;
	}

	if(x >= 50000.0)
	{
		a = 316.2278;
	}

	for(char i=0; i<7; ++i)
	{
		a = 0.5*(a+x/a);

		if(a*a == x)
		{
			break;
		}
	}

	return a;
}

void _itoa(unsigned char* str, int i, char enpty)
{  
	char sim[12] = {0};
	char len = 0;

	if(i == 0)
	{
		sim[0] = '0';
		sim[1] = '\0';
	}
	else
	{

		char minus = 0;

		if(i<0)
		{
			minus = 1;
			i=-i;
		}

		while(i>0 && len<11)
		{  
			sim[len] = (i%10) + 48;
			++len;
			i/=10;
		}

		if(minus)
		{
			sim[len] = '-';
			++len;
		}

		for(char c=0; c<len/2; ++c)
		{
			sim[11] = sim[c];
			sim[c] = sim[len-c-1];
			sim[len-c-1] = sim[11];
		}

		sim[11] = '\0';
		sim[len] = '\0';
	}

	if(enpty)
	{
		for(char c=0; c<=len; ++c)
		{
			str[c] = sim[c];
		}
	}
	else
	{
		int pos = 0;

		while(str[pos] != '\0')
		{
			pos++;
		}

		for(char c=0; c<=len; ++c)
		{
			str[pos+1+c] = sim[c];
		}

		str[pos+len+2] = '\0';
	}


}

void HAL_PCD_IRQHandler(PCD_HandleTypeDef *hpcd)
{

}

void hpcd_USB_OTG_HS()
{

}