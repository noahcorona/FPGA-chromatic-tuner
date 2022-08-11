#include "fft.h"
#include "complex.h"
#include "trig.h"

static float new_[MAX_SAMPLES];
static float new_im[MAX_SAMPLES];

static float sin_table[MAX_SAMPLES / 2];
static float cos_table[MAX_SAMPLES / 2];

void generateLUT(int samples_decimated) {
	int s2 = samples_decimated / 2;
	for(int i = 0; i < s2; i++) {
		sin_table[i] = sine(-6.283185307*i/(samples_decimated));
		cos_table[i] = cosine(-6.283185307*i/(samples_decimated));
	}
}

float fft(float* q, float* w, int n, int m, float sample_f) {
	int a = n/2,b=1,r,d,e,c,i,j,k;
	float real=0,imagine=0;

	// order
	for(i=0; i<(m-1); i++){
		d=0;
		for (j=0; j<b; j++){
			for (c=0; c<a; c++){
				e=c+d;
				new_[e]=q[(c*2)+d];
				new_im[e]=w[(c*2)+d];
				new_[e+a]=q[2*c+1+d];
				new_im[e+a]=w[2*c+1+d];
			}
			d+=(n/b);
		}
		for (r=0; r<n;r++){
			q[r]=new_[r];
			w[r]=new_im[r];
		}
		b*=2;
		a=n/(2*b);
	}
	// end order

	b=1;
	k=0;
	for (j=0; j<m; j++){	
		for(i=0; i<n; i+=2){
			if (i%(n/b)==0 && i!=0)
				k++;
			real=mult_real(q[i+1], w[i+1], cos_table[k*n/2/b], sin_table[k*n/2/b]);
			imagine=mult_im(q[i+1], w[i+1], cos_table[k*n/2/b], sin_table[k*n/2/b]);
			new_[i]=q[i]+real;
			new_im[i]=w[i]+imagine;
			new_[i+1]=q[i]-real;
			new_im[i+1]=w[i]-imagine;

		}

		// reorder
		for (i = 0; i < n / 2; i++) {
			q[i] = new_[2 * i];
			q[i + (n / 2)] = new_[2 * i + 1];
			w[i] = new_im[2 * i];
			w[i + (n / 2)] = new_im[2 * i + 1];
		}

	// end reorder	
		b*=2;
		k=0;		
	}

	// find magnitudes
	float max=0;
	int place=1;
	for(i=1;i<(n/2);i++) { 
		new_[i]=q[i]*q[i]+w[i]*w[i];
		if(max < new_[i]) {
			max=new_[i];
			place=i;
		}
	}
	
	float s = sample_f/n; // spacing of bins
	
	float frequency = s*place;

	// curve fitting for more accuarcy
	// assumes parabolic shape and uses three point to find the shift in the parabola
	// using the equation y=A(x-x0)^2+C
	float y1=new_[place-1],y2=new_[place],y3=new_[place+1];
	float x0=s+(2*s*(y2-y1))/(2*y2-y1-y3);
	x0=x0/s-1;
	
	if(x0 <0 || x0 > 2) { //error
		return 0;
	}
	if(x0 <= 1)  {
		frequency=frequency-(1-x0)*s;
	}
	else {
		frequency=frequency+(x0-1)*s;
	}
	
	return frequency;
}
