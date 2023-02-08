#include "RandLAPACK/comps/determiter.hh"
#include "blaspp.h"

#include <iostream>
#include <vector>

namespace RandLAPACK::comps::determiter {



template <typename T>
void pcg(
    int64_t m,
    int64_t n,
    const T* A,
    int64_t lda, 
    const T* b, // length m
    const T* c, // length n
    T delta, // >= 0
    std::vector<T>& resid_vec, // re
    T tol, //  > 0
    int64_t k,
    const T* M, // n-by-k
    int64_t ldm,
    const T* x0, // length n
    T* x,  // length n
    T* y // length m
    )
{
    std::vector<T> out_a1(m, 0.0);
    std::vector<T> out_at1(n, 0.0);
    std::vector<T> out_m1(n, 0.0);
    std::vector<T> out_mt1(k, 0.0);

    std::vector<T> b1(n);

    //  b1 = A'b - c
    blas::copy(n, c, 1, b1.data(), 1);
    blas::gemv(Layout::ColMajor, Op::Trans, m, n, 1.0, A, lda, b, 1, -1.0, b1.data(), 1);

    // r = b1 - (A'(A x0) + delta x0)
    //		out_a1 = A x0
    //		out_at1 = A'out_a1
    //		out_at1 += delta x0
    //		r -= out_at1
    std::vector<T> r(n, 0.0);	
    blas::copy((int)n, b1.data(),(int)1, r.data(), (int)1);
    blas::gemv(Layout::ColMajor, Op::NoTrans, m, n, 1.0, A, lda, x0, 1, delta, out_a1.data(), 1);
    blas::gemv(Layout::ColMajor, Op::Trans, m, n, 1.0, A, lda, out_a1.data(), 1, 0.0, out_at1.data(), 1);
    blas::axpy(n, delta, x0, 1, out_at1.data(), 1);
    blas::axpy(n, -1.0, out_at1.data(), 1, r.data(), 1);

    // d = M (M' r);
    std::vector<T> d(n);
    blas::gemv(Layout::ColMajor, Op::Trans, n, k, 1.0, M, ldm, r.data(), 1, 0.0, out_mt1.data(), 1);
    blas::gemv(Layout::ColMajor, Op::NoTrans, n, k, 1.0, M, ldm, out_mt1.data(), 1, 0.0, d.data(), 1);

    bool reg = delta > 0;
    blas::copy(n, x0, 1, x, 1);
    T delta1_old = blas::dot(n, d.data(), 1, r.data(), 1);
    T delta1_new = delta1_old;
    T rel_sq_tol = (delta1_old * tol) * tol;

    int64_t iter_lim = resid_vec.size();
    int64_t iter = 0;
    T alpha = 0.0;
    T beta = 0.0;
    while (iter < iter_lim && delta1_new > rel_sq_tol) {
        resid_vec[iter] = delta1_new;
        
        // q = A'(A d) + delta d 
        //		q = out_at1
        blas::gemv(Layout::ColMajor, Op::NoTrans, m, n, 1.0,  A, lda, d.data(), 1, 0.0, out_a1.data(), 1);
        blas::gemv(Layout::ColMajor, Op::Trans, m, n, 1.0, A, lda, out_a1.data(), 1, 0.0, out_at1.data(), 1);
        if (reg) blas::axpy(n, delta,  d.data(), 1, out_at1.data(), 1); 
        
        // alpha = delta1_new / (d' q)
        alpha = delta1_new / blas::dot(n, d.data(), 1, out_at1.data(), 1);
        
        // x += alpha d
        blas::axpy(n, alpha, d.data(), 1, x, 1);

        // update r
        if (iter % 25 == 1) {
            // r = b1 - (A'(A x) + delta x)
            //		out_a1 = A x
            //		out_at1 = A' out_a1
            //		r = b1
            //		r -= out_at1
            //		r -= delta x
            blas::gemv(Layout::ColMajor, Op::NoTrans, m, n, 1.0,  A, lda, x, 1, 0.0, out_a1.data(), 1);
            blas::gemv(Layout::ColMajor, Op::Trans, m, n, 1.0, A, lda, out_a1.data(), 1, 0.0, out_at1.data(), 1);
            blas::copy(n, b1.data(), 1, r.data(), 1);
            blas::axpy(n, -1.0, out_at1.data(), 1, r.data(), 1 );
            if (reg) blas::axpy(n, -delta, x, 1, r.data(), 1); 
        } else {
            // r -= alpha q
            blas::axpy(n, -alpha, out_at1.data(), 1, r.data(), 1);
        }

        // s = M (M' r)
        //		out_mt1 = M' r
        //		out_m1 = M out_mt1
        //		s = out_m1
        blas::gemv(Layout::ColMajor, Op::Trans, n, k, 1.0, M, ldm, r.data(), 1, 0.0, out_mt1.data(), 1);
        blas::gemv(Layout::ColMajor, Op::NoTrans, n, k, 1.0, M, ldm, out_mt1.data(), 1, 0.0, out_m1.data(), 1);

        // scalars and update d
        delta1_old = delta1_new;
        delta1_new = blas::dot(n, r.data(), 1, out_m1.data(), 1);
        beta = delta1_new / delta1_old;
        for (int i = 0; i < n; ++i) {
            d[i] = beta*d[i] + out_m1[i];
        }

        ++iter;
    }

    resid_vec[iter] = delta1_new;

    // recover y = b - Ax
    blas::copy(n, b, 1, y, 1);
    blas::gemv(Layout::ColMajor, Op::NoTrans, m, n, -1.0, A, lda, x, 1, 1.0, y, 1);
}


void run_pcgls_ex(int n, int m)
{
    std::vector<double> A(m * n);
    for (uint64_t i = 0; i < A.size(); ++i) {
        A[i] = ((double)i + 1.0) / m;
    }
    std::vector<double> b(m);
    for (uint64_t i = 0; i < b.size(); ++i) {
        b[i] = 1.0 / ((double) (i+1));
    }
    std::vector<double> c(n, 0.0);
    std::vector<double> M(n * n, 0.0);
    for (int i = 0; i < n; ++i) {
        M[i + n*i] = 1.0;
    }
    std::vector<double> x0(n, 0.0);
    std::vector<double> x(n, 0.0);
    std::vector<double> y(m, 0.0);
    std::vector<double> resid_vec(10*n, -1.0);

    double delta = 0.1;
    double tol = 1e-8;

    pcg<double>(m, n, A.data(), m, b.data(), c.data(), delta,
        resid_vec, tol, n, M.data(), n, x0.data(), x.data(), y.data());

    for (double res: resid_vec)
    {
        if (res < 0) {
            break;
        }
        std::cout << res << "\n";
    }
}



// Explicit instantiation of template functions - workaround to avoid header implementations
template void pcg<float>( int64_t m, int64_t n, const float* A, int64_t lda,  const float* b, const float* c, float delta, 
std::vector<float>& resid_vec, float tol, int64_t k, const float* M, int64_t ldm, const float* x0, float* x, float* y);

template void pcg<double>( int64_t m, int64_t n, const double* A, int64_t lda,  const double* b, const double* c, double delta, 
std::vector<double>& resid_vec, double tol, int64_t k, const double* M, int64_t ldm, const double* x0, double* x, double* y);

} // end namespace RandLAPACK::comps::determiter
