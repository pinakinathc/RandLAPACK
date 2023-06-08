#ifndef randlapack_comps_util_h
#define randlapack_comps_util_h

#include "rl_blaspp.hh"
#include "rl_lapackpp.hh"

#include <RandBLAS.hh>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <vector>
#include <cstdint>

namespace RandLAPACK::util {

/// An enumeration describing various matrix types by name.
/// Each matrix type can be generated bt mat_gen() utility function.
enum mat_type {polynomial, exponential, gaussian, step, spiked, adverserial, bad_cholqr};

/// A struct containing info about a given matrix to be generated by mat_gen().
/// Requires only the size and type of a matrix by default, but can have other optional parameters.
template <typename T>
struct mat_gen_info {
    int64_t rows;
    int64_t cols;
    int64_t rank;
    mat_type m_type;
    T cond_num;
    T scaling;
    bool diag;
    bool check_true_rank;

    mat_gen_info(int64_t m, int64_t n, mat_type t) {
        rows = m;
        cols = n;
        m_type = t;
        /// default values
        diag = false;
        rank = n;
        cond_num = 1.0;
        scaling = 1.0;
    }
};

/// Generates an identity matrix. Assuming col-maj
template <typename T>
void eye(
    int64_t m,
    int64_t n,
    std::vector<T>& A
) {
    T* A_dat = A.data();
    int64_t min = std::min(m, n);
    for(int j = 0; j < min; ++j) {
        A_dat[(m * j) + j] = 1.0;
    }
}

/// Diagonalization - turns a vector into a diagonal matrix. Overwrites the
/// diagonal entries of matrix S with those stored in s.
template <typename T>
void diag(
    int64_t m,
    int64_t n,
    const std::vector<T>& s,
    int64_t k, // size of s, < min(m, n)
    std::vector<T>& S // Assuming S is m by n
) {

    if(k > n) {
        // Throw an error
    }
    // size of s
    blas::copy(k, s.data(), 1, S.data(), m + 1);
}

/// Captures k diagonal elements of A and stores them in buf.
template <typename T>
void extract_diag(
    int64_t m,
    int64_t n,
    int64_t k,
    const std::vector<T>& A,
    std::vector<T>& buf
) {
    const T* A_dat = A.data();
    if (k == 0) {
        k = std::min(m, n);
    }
    for(int i = 0; i < k; ++i) {
        buf[i] = A_dat[(i * m) + i];
    }
}

/// Displays the first k diagonal elements.
template <typename T>
void disp_diag(
    int64_t m,
    int64_t n,
    int64_t k,
    const std::vector<T>& A
) {
    const T* A_dat = A.data();
    if (k == 0) {
        k = std::min(m, n);
    }
    printf("DISPLAYING THE MAIN DIAGONAL OF A GIVEN MATRIX: \n");
    for(int i = 0; i < k; ++i) {
        printf("ELEMENT %d: %f\n", i, *(A_dat + (i * m) + i));
    }
}

/// Extracts the l-portion of the GETRF result, places 1's on the main diagonal.
/// Overwrites the passed-in matrix.
template <typename T>
void get_L(
    int64_t m,
    int64_t n,
    T* A,
    int overwrite_diagonal
) {
    for(int i = 0; i < n; ++i) {
        std::fill(&A[m * i], &A[i + m * i], 0.0);
        
        if(overwrite_diagonal)
            A[i + m * i] = 1.0;
    }
}

template <typename T>
void get_L(
    int64_t m,
    int64_t n,
    std::vector<T> &L,
    int overwrite_diagonal
) {
    get_L(m, n, L.data(), overwrite_diagonal);
}

/// Stores the upper-triangualr portion of A in U.
template <typename T>
void get_U(
    int64_t m,
    int64_t n,
    const std::vector<T>& A,
    std::vector<T>& U // We are assuming U is n by n
) {
    // Vector end pointer
    int size = m * n;

    const T* A_dat = A.data();
    T* U_dat = U.data();

    for(int i = 0, j = 1, k = 0; i < size && j <= m; i += m, k +=n, ++j) {
        blas::copy(j, &A_dat[i], 1, &U_dat[k], 1);
    }
}

/// Zeros-out the lower-triangular portion of A
template <typename T>
void get_U(
    int64_t m,
    int64_t n,
    std::vector<T>& A
) {
    T* A_dat = A.data();

    for(int i = 0; i < n - 1; ++i) {
        std::fill(&A_dat[i * (m + 1) + 1], &A_dat[(i + 1) * m], 0.0);
    }
}

/// Positions columns of A in accordance with idx vector of length k.
/// idx array modified ONLY within the scope of this function.
template <typename T>
void col_swap(
    int64_t m,
    int64_t n,
    int64_t k,
    std::vector<T>& A,
    std::vector<int64_t> idx
) {

    if(k > n) {
        // Throw error
    }

    int64_t* idx_dat = idx.data();
    T* A_dat = A.data();

    int64_t i, j, l;
    for (i = 0, j = 0; i < k; ++i) {
        j = idx_dat[i] - 1;
        blas::swap(m, &A_dat[i * m], 1, &A_dat[j * m], 1);

        // swap idx array elements
        // Find idx element with value i and assign it to j
        for(l = i; l < k; ++l) {
            if(idx[l] == i + 1) {
                    idx[l] = j + 1;
                    break;
            }
        }
        idx[i] = i + 1;
    }
}


/// Checks if the given size is larger than available. If so, resizes the vector.
template <typename T>
T* upsize(
    int64_t target_sz,
    std::vector<T>& A
) {
    if ((int64_t) A.size() < target_sz)
        A.resize(target_sz, 0);

    return A.data();
}


/// Changes the number of rows of a column-major matrix.
/// Resulting array is to be k by n - THIS IS SIZING DOWN
template <typename T>
T* row_resize(
    int64_t m,
    int64_t n,
    std::vector<T>& A,
    int64_t k
) {

    T* A_dat = A.data();

    // SIZING DOWN - just moving data
    if(m > k) {
        uint64_t end = k;
        for (int i = 1; i < n; ++i) {
            // Place ith column (of k entries) after the (i - 1)st column
            blas::copy(k, &A_dat[m * i], 1, &A_dat[end], 1);
            end += k;
        }
    } else { //SIZING UP
        // How many rows are being added: k - m
        A_dat = upsize(k * n, A);

        int64_t end = k * (n - 1);
        for(int i = n - 1; i > 0; --i) {
            // Copy in reverse order to avoid overwriting
            blas::copy(m, &A_dat[m * i], -1, &A_dat[end], -1);
            std::fill(&A_dat[m * i], &A_dat[end], 0.0);
            end -= k;
        }
    }

    return A_dat;
}

/// Generates left and right singular vectors for the three matrix types above.
/// Note: Printed matrix A may have different rank from actual generated matrix A
template <typename T, typename RNG>
void gen_mat(
    int64_t m,
    int64_t n,
    std::vector<T>& A,
    int64_t k,
    std::vector<T>& S,
    RandBLAS::RNGState<RNG> state
) {

    std::vector<T> U(m * k, 0.0);
    std::vector<T> V(n * k, 0.0);
    std::vector<T> tau(k, 2.0);
    std::vector<T> Gemm_buf(m * k, 0.0);

    // Data pointer predeclarations for whatever is accessed more than once
    T* U_dat = U.data();
    T* V_dat = V.data();
    T* tau_dat = tau.data();
    T* Gemm_buf_dat = Gemm_buf.data();

    RandBLAS::DenseDist DU{.n_rows = m, .n_cols = k};
    RandBLAS::DenseDist DV{.n_rows = n, .n_cols = k};
    state = RandBLAS::fill_dense(DU, U_dat, state);
    state = RandBLAS::fill_dense(DV, V_dat, state);

    lapack::geqrf(m, k, U_dat, m, tau_dat);
    lapack::ungqr(m, k, k, U_dat, m, tau_dat);

    lapack::geqrf(n, k, V_dat, n, tau_dat);
    lapack::ungqr(n, k, k, V_dat, n, tau_dat);

    blas::copy(m * k, U_dat, 1, Gemm_buf_dat, 1);
    for(int i = 0; i < k; ++i) {
        blas::scal(m, S[i + k * i], &Gemm_buf_dat[i * m], 1);
    }

    blas::gemm(Layout::ColMajor, Op::NoTrans, Op::Trans, m, n, k, 1.0, Gemm_buf_dat, m, V_dat, n, 0.0, A.data(), m);
}

/// Generates matrix with the following singular values:
/// sigma_i = 1 / (i + 1)^pow (first k * 0.2 sigmas = 1
/// Output matrix is m by n of rank k.
/// In later case, left and right singular vectors are randomly-generated
/// and orthogonaized.
/// Boolean parameter 'diag' signifies whether the matrix is to be
/// generated as diagonal.
/// Parameter 'cond' signfies the condition number of a generated matrix.
template <typename T, typename RNG>
void gen_poly_mat(
    int64_t& m,
    int64_t& n,
    std::vector<T>& A,
    int64_t k,
    T cond,
    bool diagon,
    RandBLAS::RNGState<RNG> state
) {

    // Predeclare to all nonzero constants, start decay where needed
    std::vector<T> s(k, 1.0);
    std::vector<T> S(k * k, 0.0);

    // The first 10% of the singular values will be =1
    int offset = (int) floor(k * 0.1);

    // We have a set condition number, so need to find an exponent parameter
    // The higher the value, the faster the decay
    T t = log2(cond) / log2(k - offset);

    T cnt = 0.0;
    // apply lambda function to every entry of s
    std::for_each(s.begin() + offset, s.end(),
        // Lambda expression begins
        [&t, &cnt](T& entry) {
                entry = 1 / std::pow(++cnt, t);
        }
    );

    // form a diagonal S
    diag(k, k, s, k, S);

    if (diagon) {
        if (!(m == k || n == k)) {
            m = k;
            n = k;
            A.resize(k * k);
        }
        lapack::lacpy(MatrixType::General, k, k, S.data(), k, A.data(), k);
    } else {
        gen_mat(m, n, A, k, S, state);
    }
}

/// Generates matrix with the following singular values:
/// sigma_i = e^((i + 1) * -pow) (first k * 0.2 sigmas = 1
/// Output matrix is m by n of rank k.
/// In later case, left and right singular vectors are randomly-generated
/// and orthogonaized.
/// Boolean parameter 'diag' signifies whether the matrix is to be
/// generated as diagonal.
/// Parameter 'cond' signfies the condition number of a generated matrix.
template <typename T, typename RNG>
void gen_exp_mat(
    int64_t& m,
    int64_t& n,
    std::vector<T>& A,
    int64_t k,
    T cond,
    bool diagon,
    RandBLAS::RNGState<RNG> state
) {

    std::vector<T> s(k, 1.0);
    std::vector<T> S(k * k, 0.0);

    // The first 10% of the singular values will be =1
    int offset = (int) floor(k * 0.1);

    T t = -log(1 / cond) / (k - offset);

    T cnt = 0.0;
    // apply lambda function to every entry of s
    // Please make sure that the first singular value is always 1
    std::for_each(s.begin() + offset, s.end(),
        // Lambda expression begins
        [&t, &cnt](T& entry) {
                entry = (std::exp(++cnt * -t));
        }
    );

    // form a diagonal S
    diag(k, k, s, k, S);
    if (diagon) {
        if (!(m == k || n == k)) {
                m = k;
                n = k;
                A.resize(k * k);
        }
        lapack::lacpy(MatrixType::General, k, k, S.data(), k, A.data(), k);
    } else {
        gen_mat(m, n, A, k, S, state);
    }
}

/// Generates matrix with a staircase spectrum with 4 steps.
/// Output matrix is m by n of rank k.
/// Boolean parameter 'diag' signifies whether the matrix is to be
/// generated as diagonal.
/// Parameter 'cond' signfies the condition number of a generated matrix.
template <typename T, typename RNG>
void gen_step_mat(
    int64_t& m,
    int64_t& n,
    std::vector<T>& A,
    int64_t k,
    T cond,
    bool diagon,
    RandBLAS::RNGState<RNG> state
) {

    // Predeclare to all nonzero constants, start decay where needed
    std::vector<T> s(k, 1.0);
    std::vector<T> S(k * k, 0.0);

    // We will have 4 steps controlled by the condition number size and starting with 1
    int offset = (int) (k / 4);

    std::fill(s.begin(), s.begin() + offset, 1);
    std::fill(s.begin() + offset + 1, s.begin() + 2 * offset, 8.0 / cond);
    std::fill(s.begin() + 2 * offset + 1, s.begin() + 3 * offset, 4.0 / cond);
    std::fill(s.begin() + 3 * offset + 1, s.end(), 1.0 / cond);

    // form a diagonal S
    diag(k, k, s, k, S);

    if (diagon) {
        if (!(m == k || n == k)) {
            m = k;
            n = k;
            A.resize(k * k);
        }
        lapack::lacpy(MatrixType::General, k, k, S.data(), k, A.data(), k);
    } else {
        gen_mat(m, n, A, k, S, state);
    }
}

/// Generates a matrix with high coherence between the left singular vectors.
/// Output matrix is m by n, full-rank.
/// Such matrix would be difficult to sketch.
/// Right singular vectors are sampled uniformly at random.
template <typename T, typename RNG>
void gen_spiked_mat(
    int64_t& m,
    int64_t& n,
    std::vector<T>& A,
    T spike_scale,
    RandBLAS::RNGState<RNG> state
) {
    T* A_dat = upsize(m * n, A);

    int64_t num_rows_sampled = n / 2;

    /// sample from [m] without replacement. Get the row indices for a tall LASO with a single column.
    RandBLAS::SparseDist DS = {.n_rows = m, .n_cols = 1, .vec_nnz = num_rows_sampled, .major_axis = RandBLAS::MajorAxis::Long};
    RandBLAS::SparseSkOp<T, RNG> S(DS, state);
    RandBLAS::fill_sparse(S);

    std::vector<T> V(n * n, 0.0);
    std::vector<T> tau(n, 0.0);

    RandBLAS::DenseDist DV{.n_rows = n, .n_cols = n};
    state = RandBLAS::fill_dense(DV, V.data(), state);

    lapack::geqrf(n, n, V.data(), n, tau.data());
    lapack::ungqr(n, n, n, V.data(), n, tau.data());

    // Fill A with stacked copies of V
    int start = 0;
    while(start + n <= m){
        for(int j = 0; j < n; ++j) {
            blas::copy(n, &V[m * j], 1, &A[start + (m * j)], 1);
        }
        start += n;
    }

    // Scale randomly sampled rows
    start = 0;
    while (start + m <= m * n) {
        for(int i = 0; i < num_rows_sampled; ++i) {
            A_dat[start + (S.cols)[i] - 1] *= spike_scale;
        }
        start += m;
    }
}

/// Generates a numerically rank-deficient matrix.
/// Added per Oleg's suggestion.
/// Output matrix is m by n of some rank k < n.
/// Generates a matrix of the form A = UV, where
/// U is an m by n Gaussian matrix whose first row was scaled by a factor sigma, and that then 
/// was orthonormalized with a Householder QR. 
/// The matrix V is the upper triangular part of an n × n 
/// orthonormalized Gaussian matrix with modified diagonal entries to diag(V) *= [1, 10^-15, . . . , 10^-15, 10^-15].
template <typename T, typename RNG>
void gen_oleg_adversarial_mat(
    int64_t& m,
    int64_t& n,
    std::vector<T>& A,
    T sigma,
    RandBLAS::RNGState<RNG> state
) {

    T scaling_factor_U = sigma;
    T scaling_factor_V = 10e-3;

    std::vector<T> U(m * n, 0.0);
    std::vector<T> V(n * n, 0.0);
    std::vector<T> tau1(n, 0.0);
    std::vector<T> tau2(n, 0.0);

    RandBLAS::DenseDist DU{.n_rows = m, .n_cols = n};
    state = RandBLAS::fill_dense(DU, U.data(), state);

    RandBLAS::DenseDist DV{.n_rows = n, .n_cols = n};
    state = RandBLAS::fill_dense(DV, V.data(), state);

    T* U_dat = U.data();
    for(int i = 0; i < n; ++i) {
        //U_dat[m * i + 1] *= scaling_factor_U;
        for(int j = 0; j < 10; ++j) {
            U_dat[m * i + j] *= scaling_factor_U;
        }
    }

    lapack::geqrf(m, n, U.data(), m, tau1.data());
    lapack::ungqr(m, n, n, U.data(), m, tau1.data());

    lapack::geqrf(n, n, V.data(), n, tau2.data());
    lapack::ungqr(n, n, n, V.data(), n, tau2.data());

    // Grab an upper-triangular portion of V
    get_U(n, n, V);

    T* V_dat = V.data();
    for(int i = 11; i < n; ++i)
        V_dat[n * i + i] *= scaling_factor_V;

    blas::gemm(Layout::ColMajor, Op::NoTrans, Op::NoTrans, m, n, n, 1.0, U.data(), m, V.data(), n, 0.0, A.data(), m);
}

/// Per Oleg's suggestion, this matrix is supposed to break QB with Cholesky QR.
/// Output matrix is m by n, full-rank.
/// Parameter 'k' signifies the dimension of a sketching operator.
/// Boolean parameter 'diag' signifies whether the matrix is to be
/// generated as diagonal.
/// Parameter 'cond' signfies the condition number of a generated matrix.
template <typename T, typename RNG>
void gen_bad_cholqr_mat(
    int64_t& m,
    int64_t& n,
    std::vector<T>& A,
    int64_t k,
    T cond,
    bool diagon,
    RandBLAS::RNGState<RNG> state
) {

    std::vector<T> s(n, 1.0);
    std::vector<T> S(n * n, 0.0);

    // The first k singular values will be =1
    int offset = k;

    // Then, we start with 10^-8 and decrease exponentially
    T t = log(std::pow(10, 8) / cond) / (1 - (n - offset));

    T cnt = 0.0;
    // apply lambda function to every entry of s
    // Please make sure that the first singular value is always 1
    std::for_each(s.begin() + offset, s.end(),
        // Lambda expression begins
        [&t, &cnt](T& entry) {
                entry = (std::exp(t) / std::pow(10, 8)) * (std::exp(++cnt * -t));
        }
    );

    // form a diagonal S
    diag(k, k, s, k, S);
    if (diagon) {
        if (!(m == k || n == k)) {
                m = k;
                n = k;
                A.resize(k * k);
        }
        lapack::lacpy(MatrixType::General, k, k, S.data(), k, A.data(), k);
    } else {
        gen_mat(m, n, A, k, S, state);
    }
}

/// Find the condition number of a given matrix A.
template <typename T>
T cond_num_check(
    int64_t m,
    int64_t n,
    const std::vector<T>& A,
    std::vector<T>& A_cpy,
    std::vector<T>& s,
    bool verbose
) {

    // Copy to avoid any changes
    T* A_cpy_dat = upsize(m * n, A_cpy);
    T* s_dat = upsize(n, s);

    // Packed storage check
    if (A.size() < A_cpy.size()) {
        // Convert to normal format
        lapack::tfttr(Op::NoTrans, Uplo::Upper, n, A.data(), A_cpy_dat, m);
    } else {
        lapack::lacpy(MatrixType::General, m, n, A.data(), m, A_cpy_dat, m);
    }
    lapack::gesdd(Job::NoVec, m, n, A_cpy_dat, m, s_dat, NULL, m, NULL, n);

    T cond_num = s_dat[0] / s_dat[n - 1];

    if (verbose)
        printf("CONDITION NUMBER: %f\n", cond_num);

    return cond_num;
}

// Computes the numerical rank of a given matirx
template <typename T>
int64_t rank_check(
    int64_t m,
    int64_t n,
    const std::vector<T>& A
) {
    std::vector<T> A_pre_cpy;
    std::vector<T> s;
    RandLAPACK::util::cond_num_check(m, n, A, A_pre_cpy, s, false);

    for(int i = 0; i < n; ++i) {
        if (s[i] / s[0] <= 5 * std::numeric_limits<T>::epsilon())
            return i - 1;
    }
    return n;
}

/// Dimensions m and n may change if we want the diagonal matrix of rank k < min(m, n).
/// In that case, it would be of size k by k.
template <typename T, typename RNG>
void mat_gen(
    mat_gen_info<T> info,
    std::vector<T>& A,
    RandBLAS::RNGState<RNG> state
) {
    // Base parameters
    int64_t m = info.rows;
    int64_t n = info.cols;
    int64_t k = info.rank;
    T* A_dat = A.data();

    switch(info.m_type) {
        /*
        First 3 cases are identical, varying ony in the entry generation function.
        is there a way to propagate the lambda expression or somehowe pass several parameners into a undary function in foreach?
        */
        case polynomial:
                // Generating matrix with polynomially decaying singular values
                RandLAPACK::util::gen_poly_mat(m, n, A, k, info.cond_num, info.diag, state);
                break;
        case exponential:
                // Generating matrix with exponentially decaying singular values
                RandLAPACK::util::gen_exp_mat(m, n, A, k, info.cond_num, info.diag, state);
                break;
            break;
        case gaussian: {
                // Gaussian random matrix
                RandBLAS::DenseDist D{.n_rows = m, .n_cols = n};
                RandBLAS::fill_dense(D, A_dat, state);
            }
            break;
        case step: {
                // Generating matrix with a staircase-like spectrum
                RandLAPACK::util::gen_step_mat(m, n, A, k, info.cond_num, info.diag, state);
            }    
            break;
        case spiked: {
                // This matrix may be numerically rank deficient
                RandLAPACK::util::gen_spiked_mat(m, n, A, info.scaling, state);
                if(info.check_true_rank)
                    k = rank_check(m, n, A);
            }
            break;
        case adverserial: {
                // This matrix may be numerically rank deficient
                RandLAPACK::util::gen_oleg_adversarial_mat(m, n, A, info.scaling, state);
                if(info.check_true_rank)
                    k = rank_check(m, n, A);
            }
            break;
        case bad_cholqr: {
                // Per Oleg's suggestion, this is supposed to make QB fail with CholQR for orth/stab
                RandLAPACK::util::gen_bad_cholqr_mat(m, n, A, k, info.cond_num, info.diag, state);
            }
            break;
        default:
            throw std::runtime_error(std::string("Unrecognized case."));
            break;
    }
}

/// Checks whether matrix A has orthonormal columns.
template <typename T>
bool orthogonality_check(
    int64_t m,
    int64_t n,
    int64_t k,
    const std::vector<T>& A,
    std::vector<T>& A_gram,
    bool verbose
) {

    const T* A_dat = A.data();
    T* A_gram_dat = A_gram.data();

    blas::syrk(Layout::ColMajor, Uplo::Lower, Op::Trans, n, m, 1.0, A_dat, m, 0.0, A_gram_dat, n);

    for (int oi = 0; oi < k; ++oi) {
        A_gram_dat[oi * n + oi] -= 1.0;
    }
    T orth_err = lapack::lange(Norm::Fro, n, n, A_gram_dat, k);

    if(verbose) {
        printf("Q ERROR:   %e\n\n", orth_err);
    }

    if (orth_err > 1.0e-10)
        return true;

    return false;
}

/// Computes an L-2 norm of a given matrix using
/// p steps of power iteration.
template <typename T, typename RNG>
T estimate_spectral_norm(
    int64_t m,
    int64_t n,
    T const* A_dat,
    int p,
    RandBLAS::RNGState<RNG> state
) {

    std::vector<T> buf (n, 0.0);
    std::vector<T> buf1 (m, 0.0);

    RandBLAS::DenseDist DV{.n_rows = n, .n_cols = 1};
    state = RandBLAS::fill_dense(DV, buf.data(), state);

    T prev_norm_inv = 1.0;
    for(int i = 0; i < p; ++i) {
        // A * v
        gemv(Layout::ColMajor, Op::NoTrans, m, n, 1.0, A_dat, m, buf.data(), 1, 0.0, buf1.data(), 1);
        // prev_norm_inv * A' * A * v
        gemv(Layout::ColMajor, Op::Trans, m, n, prev_norm_inv, A_dat, m, buf1.data(), 1, 0.0, buf.data(), 1);
        prev_norm_inv = 1 / blas::nrm2(n, buf.data(), 1);
    }

    return std::sqrt(blas::nrm2(n, buf.data(), 1));
}

/// Uses recursion to find the rank of the matrix pointed to by A_dat.
/// Does so by attempting to find the smallest k such that 
/// ||A[k:, k:]||_F <= tau_trunk * ||A||.
/// ||A|| can be either 2 or Fro.
/// Finding such k is done via binary search in range [1, n], which is 
/// controlled by ||A[k:, k:]||_F (<)(>) tau_trunk * ||A||. 
/// We first attempt to find k that results in an expression closest to 
/// ||A[k:, k:]||_F == tau_trunk * ||A|| and then ensure that ||A[k:, k:]||_F
/// is not smaller than tau_trunk * ||A|| to avoid rank underestimation.
template <typename T>
int64_t rank_search_binary(
    int64_t lo,
    int64_t hi,
    int64_t k,
    int64_t n,
    T norm_A,
    T tau_trunc,
    T const* A_dat
) {
    T norm_R_sub = lapack::lange(Norm::Fro, n - k, n, &A_dat[k * n], n - k);

    if(((k - lo) / 2) == 0) {
        // Need to make sure we are not underestimating rank
        while(norm_R_sub > tau_trunc * norm_A)
        {
            ++k;
            norm_R_sub = lapack::lange(Norm::Fro, n - k, n, &A_dat[k * n], n - k);
        }
        return k;
    } else if (norm_R_sub > tau_trunc * norm_A) {
        // k is larger
        k = rank_search_binary(k, hi, k + ((k - lo) / 2), n, norm_A, tau_trunc, A_dat);
    } else { //(norm_R_sub < tau_trunc * norm_A) {
        // k is smaller
        k = rank_search_binary(lo, k, lo + ((k - lo) / 2), n, norm_A, tau_trunc, A_dat);
    }
    return k;
}

/// Normalizes columns of a given matrix, writes the result into a buffer
template <typename T>
void normc(
    int64_t m,
    int64_t n,
    const std::vector<T>& A,
    std::vector<T>& A_norm
) {
    util::upsize(m * n, A_norm);

    T col_nrm = 0.0;
    for(int i = 0; i < n; ++i) {
        col_nrm = blas::nrm2(m, &A[m * i], 1);
        if(col_nrm != 0) {
            for (int j = 0; j < m; ++j) {
                A_norm[m * i + j] = A[m * i + j] / col_nrm;
            }
        }
    }
}


/**
 * In-place transpose of square matrix of order n, with leading dimension n.
 * Turns out that "layout" doesn't matter here.
*/
template <typename T>
void transpose_square(T* H, int64_t n) {
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            T val_ij = H[i + j*n];
            T val_ji = H[j + i*n];
            H[i + j*n] = val_ji;
            H[j + i*n] = val_ij;
        }
    }
    return;
}

/**
 * 
*/
template <typename T>
void eat_lda_slack(
    T* buff,
    int64_t vec_len,
    int64_t num_vecs,
    int64_t inter_vec_stride
) {
    if (vec_len == inter_vec_stride)
        return;
    T* work = new T[vec_len]{};
    for (int i = 0; i < num_vecs; ++i) {
        blas::copy(vec_len, &buff[i*inter_vec_stride], 1, work, 1);
        blas::copy(vec_len, work, 1, &buff[i*vec_len], 1);
    }
    delete [] work;
}

} // end namespace util
#endif
