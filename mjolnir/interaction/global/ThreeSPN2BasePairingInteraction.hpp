#ifndef MJOLNIR_INTERACTION_GLOBAL_3SPN_BASE_BASE_INTERACTION_HPP
#define MJOLNIR_INTERACTION_GLOBAL_3SPN_BASE_BASE_INTERACTION_HPP
#include <mjolnir/potential/global/ThreeSPN2BasePairingPotential.hpp>
#include <mjolnir/potential/global/ThreeSPN2CrossStackingPotential.hpp>
#include <mjolnir/core/SimulatorTraits.hpp>
#include <memory>

namespace mjolnir
{

// 3SPN.2 Base-Base non-local interaction (base pairing & cross-stacking).
// This is an implementation of the potential developed in the following paper.
// - D.M.Hinckley, G.S.Freeman, J.K.Whitmer, and J.J.de Pablo (2013) J. Chem. Phys.
//   doi: 10.1063/1.4822042
//
// This interaction is deeply coupled with its potential, so it does not receive
// a potential type as a template argument. It always uses the same potential,
// `ThreeSPN2BasePairingPotential` and `ThreeSPN2CrossStackingPotential`.
//
// It shares CellList between BasePairing and CrossStacking.
// It constructs CellList with BasePairingPotential and re-use the pairs in
// CrossStacking.
//
template<typename traitsT, typename partitionT>
class ThreeSPN2BaseBaseInteraction final : public GlobalInteractionBase<traitsT>
{
  public:

    using traits_type     = SimulatorTraits<realT, boundaryT>;
    using base_type       = GlobalInteractionBase<traits_type>;
    using real_type       = typename base_type::real_type;
    using coordinate_type = typename base_type::coordinate_type;
    using system_type     = typename base_type::system_type;
    using boundary_type   = typename base_type::boundary_type;
    using partition_type  = partitionT;

    using base_pairing_potential   = ThreeSPN2BasePairingPotential<real_type>;
    using cross_stacking_potential = ThreeSPN2CrossStackingPotential<real_type>;

  public:
    ThreeSPN2BaseBaseInteraction()  = default;
    ~ThreeSPN2BaseBaseInteraction() = default;

    ThreeSPN2BaseBaseInteraction(potential_type&& pot, partition_type&& part)
        : potential_(std::move(pot)), partition_(std::move(part))
    {}

    void initialize(const system_type& sys) override
    {
        MJOLNIR_GET_DEFAULT_LOGGER();
        MJOLNIR_LOG_FUNCTION();
        MJOLNIR_LOG_INFO("potential is ", this->name());
        this->potential_.initialize(sys);
        this->partition_.initialize(sys, this->potential_);
    }

    void update(const system_type& sys) override
    {
        MJOLNIR_GET_DEFAULT_LOGGER();
        MJOLNIR_LOG_FUNCTION();
        MJOLNIR_LOG_INFO("potential is ", this->name());
        this->potential_.update(sys);
        this->partition_.initialize(sys, this->potential_);
    }

    void update_margin(const real_type dmargin, const system_type& sys) override
    {
        this->partition_.update(dmargin, sys, this->potential_);
        return;
    }

    void      calc_force (system_type& sys)       const noexcept override;
    real_type calc_energy(const system_type& sys) const noexcept override;

    std::string name() const override {return "3SPN2BaseBaseInteraction";}

  private:

    base_pairing_potential   base_pairing_potential_;
    cross_stacking_potential cross_stacking_potential_;
    partition_type           partition_;
};

template<typename traitsT, typename partitionT>
void ThreeSPN2BaseBaseInteraction<traitsT, partitionT>::calc_force(
        system_type& sys) const noexcept
{
    constexpr auto pi     = math::constants<real_type>::pi;
    constexpr auto two_pi = math::constants<real_type>::two_pi;

    for(const auto i : this->potential_.participants())
    {
        const auto& rBi = sys.position(i);
        for(const auto& ptnr : this->partition_.partners(i))
        {
            const auto  j    = ptnr.index;
            const auto& para = ptnr.parameter();
            const auto& rBj  = sys.position(j);
            const auto  bp_kind = para.bp_kind;

            const auto Bij = sys.adjust_direction(rBj - rBi); // Bi -> Bj
            const auto lBij_sq = math::length_sq(Bij);
            if(lBij_sq > base_pairing_potential_.cutoff_sq(bp_kind))
            {
                continue;
            }
            // ================================================================
            // base pairing
            //
            //  Si o         o Sj
            //      \-.   ,-/
            //    Bi o =(= o Bj
            //
            // U_rep(rij) + 1/2(1+cos(dphi)) f(dtheta1) f(dtheta2) U_attr(rij)

            const auto rlBij = math::rsqrt(lBij_sq); // 1 / |Bij|
            const auto lBij  = lBij_sq * rlBij;      // |Bij|

            const auto Bij_reg =  rlBij * Bij;
            const auto Bji_reg = -rlBij * Bij;

            // ----------------------------------------------------------------
            // calculate the the repulsive part, which does not depend on angle.
            //
            // dU_rep = 2 a e exp(-a(r-r0)) (1-exp(-a(r-r0))) ... r  <  r0
            //        = 0                                     ... r0 <= r
            //
            const auto r0   = base_pairing_potential_.r0(bp_kind);
            const auto e_BP = base_pairing_potential_.epsilon(bp_kind);
            const auto a_BP = base_pairing_potential_.alpha();
            if(lBij < r0)
            {
                const auto term = std::exp(-a_BP * (lBij - r0));
                const auto coef = 2 * a_BP * e_BP * term * (real_type(1) - term);
                sys.force(Bi) += coef * Bji_reg;
                sys.force(Bj) += coef * Bij_reg;
            }

            // ----------------------------------------------------------------
            // calc theta1 and 2 to calculate the attractive part,
            //  = 1/2(1+cos(dphi)) f(dtheta1) f(dtheta2) U_attr(rij)
            //
            //   theta1   theta2
            //       |     |
            //  Si o v     v o Sj
            //      \-.   ,-/
            //    Bi o =(= o Bj

            const auto   Si = para.Si;
            const auto   Sj = para.Sj;
            const auto& rSi = sys.position(Si);
            const auto& rSj = sys.position(Sj);

            const auto SBi = sys.adjust_direction(rBi - rSi); // Si -> Bi
            const auto SBj = sys.adjust_direction(rBj - rSj); // Sj -> Bj

            const auto lSBi_sq = math::length_sq(SBi); // |SBi|^2
            const auto lSBj_sq = math::length_sq(SBj); // |SBj|^2
            const auto rlSBi   = math::rsqrt(lSBi_sq); // 1 / |SBi|
            const auto rlSBj   = math::rsqrt(lSBi_sq); // 1 / |SBj|
            const auto BSi_reg = -rlSBi * SBi;
            const auto BSj_reg = -rlSBj * SBj;

            const auto dot_SBiBj  = -math::dot_product(SBi, Bij);
            const auto dot_SBjBi  =  math::dot_product(SBj, Bij);
            const auto cos_theta1 = dot_SBiBj * rlSBi * rlBij;
            const auto cos_theta2 = dot_SBjBi * rlSBj * rlBij;
            const auto theta1 = std::acos(math::clamp<real_type>(cos_theta1, -1, 1));
            const auto theta2 = std::acos(math::clamp<real_type>(cos_theta2, -1, 1));

            // ----------------------------------------------------------------
            // calc angle-dependent terms and advance if both are nonzero
            //
            // 1/2(1+cos(dphi)) f(dtheta1) f(dtheta2) U_attr(rij)

            const auto f1 = base_pairing_potential_.f(bp_kind, theta1);
            const auto f2 = base_pairing_potential_.f(bp_kind, theta2);

            if(f1 != real_type(0.0) && f2 != real_type(0.0))
            {
                // calculate dihedral, phi
                //
                //  Si o         o Sj
                //      \       /
                //    Bi o =(= o Bj
                //         phi

                const auto df1 = base_pairing_potential_.df(bp_kind, theta1);
                const auto df2 = base_pairing_potential_.df(bp_kind, theta2);

                const auto rlBij_sq = rlBij * rlBij; // 1 / |Bij|^2
                const auto R = -SBi + (-dot_SBiBj * rlBij_sq) * Bij;
                const auto S = -SBj + ( dot_SBjBi * rlBij_sq) * Bij;

                const auto dot_phi = math::dot_product(R, S) * math::rsqrt(
                                         math::length_sq(R) * math::length_sq(S));
                const auto cos_phi = math::clamp<real_type>(dot_phi, -1, 1);

                const auto m = math::cross_product(-SBi, Bij);
                const auto n = math::cross_product( Bij, SBj);

                const auto phi = std::copysign(std::acos(cos_phi),
                                               -math::dot_product(SBi, n));

                auto dphi = phi - this->potential_.phi_0(BP_kind);
                if(dphi < -pi) {dphi += two_pi;}
                if(pi <= dphi) {dphi -= two_pi;}
                const auto cos_dphi = std::cos(dphi);
                const auto sin_dphi = std::sin(dphi);

                // ------------------------------------------------------------
                // calculate attractive force
                //
                // d/dr [1/2 (1 + cos(dphi)) f(dtheta1) f(dtheta2) U_attr(Bij)]
                // = ( -sin(dphi))/2 f(dtheta1) f(dtheta2) U_attr(Bij) dphi/dr
                // + (1+cos(dphi))/2 df/dtheta1 f(dtheta2) U_attr(Bij) dtheta1/dr
                // + (1+cos(dphi))/2 f(dtheta1) df/dtheta2 U_attr(Bij) dtheta2/dr
                // + (1+cos(dphi))/2 f(dtheta1) f(dtheta2) dU_attr/dr  dBij/dr

                if(cos_dphi != real_type(-1.0))
                {
                    // --------------------------------------------------------
                    // U_m^attr =
                    //   -e                             ... (dBij <= dBij0)
                    //   -e + e * (1 - exp(-a(r-r0)))^2 ... (otherwise)
                    //
                    // dU_m^attr / dr =
                    //   0                                 ... (dBij <= dBij0)
                    //   2ae(1-exp(-a(r-r0)))exp(-a(r-r0)) ... (otherwise)
                    //
                    real_type Um_attr_  = -e_BP;
                    real_type dUm_attr_ =     0;
                    if(lBij > r0)
                    {
                        const auto expm = std::exp(-a_BP * (dBij - r0));
                        Um_attr_  += e_BP * (real_type(1.0) - expm) *
                                            (real_type(1.0) - expm);
                        dUm_attr_  = 2 * a_BP * e_BP * expm * (1 - expm);
                    }
                    const auto Um_attr  = Um_attr_;  // mark them const
                    const auto dUm_attr = dUm_attr_; // mark them const

                    // --------------------------------------------------------
                    // calc dihedral term
                    // -sin(dphi)/2 f(dtheta1) f(dtheta2) U_attr(Bij) dphi/dr
                    if(sin_dphi != real_type(0.0))
                    {
                        const auto coef = real_type(-0.5) * sin_dphi * f1 * f2 * Um_attr;

                        const auto fSi = ( coef * lBij / math::length_sq(m)) * m;
                        const auto fSj = (-coef * lBij / math::length_sq(n)) * n;

                        const auto coef_Bi = dot_SBiBj * rlBij_sq;
                        const auto coef_Bj = dot_SBjBi * rlBij_sq;

                        sys.force(Si) += fSi;
                        sys.force(Bi) += (coef_Bi - real_type(1.0)) * fSi - coef_Bj * fSj;
                        sys.force(Bj) += (coef_Bj - real_type(1.0)) * fSj - coef_Bi * fSi;
                        sys.force(Sj) += fSj;
                    }

                    const auto dihd_term = real_type(0.5) * (real_type(1.0) + cos_dphi);

                    // --------------------------------------------------------
                    // calc theta1 term
                    // (1+cos(dphi))/2 df/dtheta1 f(dtheta2) U_attr(Bij) dtheta1/dr
                    if(df1 != real_type(0.0))
                    {
                        const auto coef = dihd_term * df1 * f2 * Um_attr;

                        const auto sin_theta1 = std::sin(theta1);
                        const auto coef_rsin  = (sin_theta1 > tolerance) ?
                                   (coef / sin_theta1) : (coef / tolerance);

                        const auto fSi = (coef_rsin * rlSBi) *
                                         (cos_theta1 * BSi_reg - Bij_reg);
                        const auto fBj = (coef_rsin * rlBij) *
                                         (cos_theta1 * Bij_reg - BSi_reg);
                        sys.force(Si) += fSi;
                        sys.force(Bi) -= (fSi + fBj);
                        sys.force(Bj) += fBj;
                    }
                    // --------------------------------------------------------
                    // calc theta2 term
                    // (1+cos(dphi))/2 f(dtheta1) df/dtheta2 U_attr(Bij) dtheta2/dr
                    if(df2 != real_type(0.0))
                    {
                        const auto coef = dihd_term * f1 * df2 * Um_attr;

                        const auto sin_theta2 = std::sin(theta2);
                        const auto coef_rsin  = (sin_theta2 > tolerance) ?
                                   (coef / sin_theta2) : (coef / tolerance);

                        const auto fBi = (coef_rsin * rlBij) *
                                         (cos_theta2 * Bji_reg - BSj_reg);
                        const auto fSj = (coef_rsin * rlSBj) *
                                         (cos_theta2 * BSj_reg - Bji_reg);
                        sys.force(Bi) += fBi;
                        sys.force(Bj) -= (fBi + fSi);
                        sys.force(Sj) += fSj;
                    }
                    // --------------------------------------------------------
                    // calc distance
                    // + 1/2 (1+cos(dphi)) f(dtheta1) f(dtheta2) dU_attr/dr  dBij/dr
                    if(dUm_attr != real_type(0.0))
                    {
                        const auto coef = dihd_term * f1 * f2 * dUm_attr;
                        sys.force(Bi) -= coef * Bij_reg;
                        sys.force(Bj) -= coef * Bji_reg;
                    }
                }
            }

            // ================================================================
            // cross stacking
            // f(theta_3) f(theta_CS) U_attr(epsilon, alpha, rij)
            //
            //       Si   Bi   Bj   Sj
            //  5'    o -- o===o -- o     3'
            //  ^    /      \ /      \    |
            //  | P o        x        o P |
            //  |    \      / \      /    v
            //  3'    o -- o===o -- o     5'
            //           Bi3   Bj5
            //
            // d/dr Vcs =
            //    df/dtheta3 f(theta_CS)  U_attr(eps, alp, rij) dtheta_3  /dr
            //  + f(theta_3) df/dtheta_CS U_attr(eps, alp, rij) dtheta_CS /dr
            //  + f(theta_3) f(theta_CS)  dU_attr/drij          drij/dr
            //

            const auto Bi3 = cross_stacking_potential_.parameters()[Bi].B3;
            const auto Bj5 = cross_stacking_potential_.parameters()[Bj].B5;

            const bool Bi3_exists = (Bi3 != cross_stacking_potential_.invalid());
            const bool Bj5_exists = (Bj5 != cross_stacking_potential_.invalid());

            if(!Bi3_exists && !Bj5_exists)
            {
                continue; // if both interacting pair do not exist, do nothing.
            }

            const auto dot_SBiSBj = math::dor_product(SBi, SBj);
            const auto cos_theta3 = dot_SBiSBj * rlSBi * rlSBj;
            const auto theta3     = std::acos(math::clamp<real_type>(cos_theta3, -1, 1));
            const auto f3         = cross_stacking_potential_.f(bp_kind, theta3);
            if(f3 == real_type(0))
            {
                // f(theta) == 0 means df(theta) is also zero.
                // so here, both cross-stacking becomes zero. skip them.
                continue;
            }
            const auto df3 = cross_stacking_potential_.df(bp_kind, theta3);

            // ----------------------------------------------------------------
            // calc common part (same in between 3' and 5'), dtheta3/dr.
            const auto sin_theta3  = std::sin(cos_theta3);
            const auto rsin_theta3 = (sin_theta3 > tolerance) ?
                    (real_type(1) / sin_theta3) : (real_type(1) / tolerance);

            const auto fSi_theta3 = rsin_theta3 * rlSBi * (cos_theta3 * BSi_reg - BSj_reg);
            const auto fSj_theta3 = rsin_theta3 * rlSBj * (cos_theta3 * BSj_reg - BSi_reg);
            const auto fBi_theta3 = real_type(-1) * fSi_theta3;
            const auto fBj_theta3 = real_type(-1) * fSj_theta3;

            // 5' adjacent of Base j might not exist, e.g., edge of the DNA.
            if(Bj5_exists)
            {
                // ------------------------------------------------------------
                // 5' cross stacking
                //
                //       Si   Bi   Bj   Sj
                //  5'    o--> o===o <--o     3'
                //  ^    /   `--\        \    |
                //  | P o   tCS  \        o P |
                //  |    \        \      /    v
                //  3'    o -- o===o -- o     5'
                //           Bi3   Bj5

                const auto cs_kind = cross_stacking_potential_.cs5_kind(Bi, Bj5);
                const auto& rBj5 = sys.position(Bj5);

                const auto Bj5i     = sys.adjust_direction(rBi - rBj5);
                const auto lBj5i_sq = math::length_sq(lBj5i); // |Bj5i|^2
                const auto rlBj5i   = math::rsqrt(lBj5i_sq);  // 1 / |Bj5i|

                const auto dot_theta_CS = math::dot_product(SBi, Bj5i);
                const auto cos_theta_CS = dot_theta_CS * rlSBi * rlBj5i;
                const auto theta_CS     =
                    std::acos(math::clamp<real_type>(cos_theta_CS, -1, 1));

                const auto fCS  = cross_stacking_potential_.f(cs_kind, theta_CS);
                // if f == 0, df is also zero. if fCS == 0, no force there
                if(fCS != real_type(0))
                {
                    const auto dfCS  = cross_stacking_potential_.df(cs_kind, theta_CS);

                    // --------------------------------------------------------
                    // U_attr =
                    //   -e                             ... (dr <= dr0)
                    //   -e + e * (1 - exp(-a(r-r0)))^2 ... (dr0 < dr)
                    //
                    // dU_attr / dr =
                    //   0                                 ... (dr <= dr0)
                    //   2ae(1-exp(-a(r-r0)))exp(-a(r-r0)) ... (dr0 < dr)
                    //
                    const auto e_CS  = cross_stacking_potential_.epsilon(cs_kind);
                    const auto a_CS  = cross_stacking_potential_.alpha();
                    const auto r0_CS = cross_stacking_potential_.r0(cs_kind);
                    const auto lBj5i = lBj5i_sq * rlBj5i;

                    real_type U_attr_  = -e_CS;
                    real_type dU_attr_ =     0;
                    if(r0 < lBij)
                    {
                        const auto term = std::exp(-a_CS * (lBj5i - r0_CS));
                        U_attr_ += e_CS * (real_type(1) - term) * (real_type(1) - term);
                        dU_attr_ = 2 * a_CS * e_CS * (real_type(1) - term) * term;
                    }
                    const auto U_attr  = U_attr_;  // mark it const
                    const auto dU_attr = dU_attr_; // mark it const

                    // --------------------------------------------------------
                    // df/dtheta3 f(theta_CS)  U_attr(eps, alp, rij) dtheta_3 /dr
                    if(df3 != real_type(0))
                    {
                        const auto coef = df3 * fCS * U_attr;
                        sys.force(Si) += coef * fSi_theta3;
                        sys.force(Sj) += coef * fSj_theta3;
                        sys.force(Bi) += coef * fBi_theta3;
                        sys.force(Bj) += coef * fBj_theta3;
                    }
                    // --------------------------------------------------------
                    // f(theta_3) df/dtheta_CS U_attr(eps, alp, rij) dtheta_CS/dr
                    if(dfCS != real_type(0))
                    {
                        const auto coef         = f3 * dfCS * U_attr;
                        const auto sin_theta_CS = std::sin(theta_CS);
                        const auto coef_rsin    = (sin_theta_CS > tolerance) ?
                                   (coef / sin_theta_CS) : (coef / tolerance);
                        const auto Bj5i_reg     = rlBj5i * Bj5i;

                        const auto fSi  = coef_rsin * rlSBi  * (-cos_theta_CS * BSi_reg - Bj5i_reg);
                        const auto fBj5 = coef_rsin * rlBj5i * ( cos_theta_CS * Bj5i_reg + BSi_reg);

                        sys.force(Si)  += fSi;
                        sys.force(Bi)  -= (fSi + fBj5);
                        sys.force(Bj5) += fBj5;
                    }
                    // --------------------------------------------------------
                    // f(theta_3) f(theta_CS)  dU_attr/drij          drij/dr
                    const auto coef = f3 * fCS * dU_attr * rlBj5i;
                    sys.force(Bi)  -= coef * Bj5i;
                    sys.force(Bj5) += coef * Bj5i;
                }
            }
            // 3' adjacent of Base i might not exist, e.g., edge of the DNA.
            if(Bi3_exists)
            {
                // ------------------------------------------------------------
                // 3' cross stacking
                // f(theta_3) f(theta_CS) U_attr(epsilon, alpha, rij)
                //
                //       Si   Bi   Bj   Sj
                //  5'    o--> o===o <--o     3'
                //  ^    /        /--'   \    |
                //  | P o        /  tCS   o P |
                //  |    \      /        /    v
                //  3'    o -- o===o -- o     5'
                //           Bi3   Bj5

                const auto cs_kind = cross_stacking_potential_.cs3_kind(Bj, Bi3);
                const auto& rBi3   = sys.position(Bi3);

                const auto Bi3j     = sys.adjust_direction(rBj - rBi3);
                const auto lBi3j_sq = math::length_sq(lBi3j); // |Bi3j|^2
                const auto rlBi3j   = math::rsqrt(lBi3j_sq);  // 1 / |Bi3j|

                const auto dot_theta_CS = math::dot_product(SBj, Bi3j);
                const auto cos_theta_CS = dot_theta_CS * rlSBj * rlBi3j;
                const auto theta_CS     =
                    std::acos(math::clamp<real_type>(cos_theta_CS, -1, 1));

                const auto fCS = cross_stacking_potential_.f(cs_kind, theta_CS);
                // if f == 0, df is also zero. if fCS == 0, no force there
                if(fCS != real_type(0))
                {
                    const auto dfCS  = cross_stacking_potential_.df(cs_kind, theta_CS);

                    // --------------------------------------------------------
                    // U_attr =
                    //   -e                             ... (dr <= dr0)
                    //   -e + e * (1 - exp(-a(r-r0)))^2 ... (dr0 < dr)
                    //
                    // dU_attr / dr =
                    //   0                                 ... (dr <= dr0)
                    //   2ae(1-exp(-a(r-r0)))exp(-a(r-r0)) ... (dr0 < dr)
                    //
                    const auto e_CS  = cross_stacking_potential_.epsilon(cs_kind);
                    const auto a_CS  = cross_stacking_potential_.alpha();
                    const auto r0_CS = cross_stacking_potential_.r0(cs_kind);
                    const auto lBi3j = lBi3j_sq * rlBi3j;

                    real_type U_attr_  = -e_CS;
                    real_type dU_attr_ =     0;
                    if(r0 < lBij)
                    {
                        const auto term = std::exp(-a_CS * (lBi3j - r0_CS));
                        U_attr_ += e_CS * (real_type(1) - term) * (real_type(1) - term);
                        dU_attr_ = 2 * a_CS * e_CS * (real_type(1) - term) * term;
                    }
                    const auto U_attr  = U_attr_;  // mark it const
                    const auto dU_attr = dU_attr_; // mark it const

                    // --------------------------------------------------------
                    // df/dtheta3 f(theta_CS)  U_attr(eps, alp, rij) dtheta_3 /dr
                    if(df3 != real_type(0))
                    {
                        const auto coef = df3 * fCS * U_attr;
                        sys.force(Si) += coef * fSi_theta3;
                        sys.force(Sj) += coef * fSj_theta3;
                        sys.force(Bi) += coef * fBi_theta3;
                        sys.force(Bj) += coef * fBj_theta3;
                    }
                    // --------------------------------------------------------
                    // f(theta_3) df/dtheta_CS U_attr(eps, alp, rij) dtheta_CS/dr
                    if(dfCS != real_type(0))
                    {
                        const auto coef         = f3 * dfCS * U_attr;
                        const auto sin_theta_CS = std::sin(theta_CS);
                        const auto coef_rsin    = (sin_theta_CS > tolerance) ?
                                   (coef / sin_theta_CS) : (coef / tolerance);
                        const auto Bi3j_reg     = rlBi3j * Bi3j;

                        const auto fSj  = coef_rsin * rlSBj  *
                                          (-cos_theta_CS * BSj_reg - Bi3j_reg);
                        const auto fBi3 = coef_rsin * rlBi3j *
                                          ( cos_theta_CS * Bi3j_reg + BSj_reg);
                        sys.force(Sj)  += fSj;
                        sys.force(Bj)  -= (fSj + fBi3);
                        sys.force(Bi3) += fBi3;
                    }
                    // --------------------------------------------------------
                    // f(theta_3) f(theta_CS)  dU_attr/drij          drij/dr
                    const auto coef = f3 * fCS * dU_attr * rlBi3j;
                    sys.force(Bj)  -= coef * Bi3j;
                    sys.force(Bi3) += coef * Bi3j;
                }
            }
        }
    }
    return;
}

template<typename traitsT, typename partitionT>
typename ThreeSPN2BaseBaseInteraction<traitsT, partitionT>::real_type
ThreeSPN2BaseBaseInteraction<traitsT, partitionT>::calc_energy(
        const system_type& sys) const noexcept
{
    constexpr auto pi     = math::constants<real_type>::pi;
    constexpr auto two_pi = math::constants<real_type>::two_pi;

    real_type E = 0.0;
    for(const auto i : this->potential_.participants())
    {
        const auto& rBi = sys.position(i);

        for(const auto& ptnr : this->partition_.partners(i))
        {
            const auto  j    = ptnr.index;
            const auto& para = ptnr.parameter();
            const auto& rBj  = sys.position(j);

            const auto  bp_kind = para.bp_kind;

            const auto Bij = sys.adjust_direction(rBj - rBi); // Bi -> Bj

            const auto lBij_sq = math::length_sq(Bij);
            if(lBij_sq > base_pairing_potential_.cutoff_sq(bp_kind))
            {
                continue;
            }

            // ----------------------------------------------------------------
            // base pairing
            //
            //  Si o         o Sj
            //      \-.   ,-/
            //    Bi o =(= o Bj
            //
            // U_rep(rij) + 1/2(1+cos(dphi)) f(dtheta1) f(dtheta2) U_attr(rij)

            const auto rlBij = math::rsqrt(lBij_sq); // 1 / |Bij|
            const auto lBij  = lBij_sq * rlBij;      // |Bij|

            // ----------------------------------------------------------------
            // U_rep = e_ij (1 - exp(-a_ij (rij - r0_ij)))^2 ... rij < r0_ij
            //       = 0                                     ... r0_ij <= rij
            const auto r0   = base_pairing_potential_.r0(bp_kind);
            const auto e_BP = base_pairing_potential_.epsilon(bp_kind);
            const auto a_BP = base_pairing_potential_.alpha();
            if(lBij < r0)
            {
                const auto term = real_type(1.0) - std::exp(-a_BP * (lBij - r0));
                E += e_BP * term * term;
            }

            // ----------------------------------------------------------------
            // calc theta1 and 2
            //
            //   theta1  theta2
            //       |    |
            //  Si o v    v o Sj
            //      \-.  ,-/
            //    Bi o == o Bj

            const auto   Si = para.Si;
            const auto   Sj = para.Sj;
            const auto& rSi = sys.position(Si);
            const auto& rSj = sys.position(Sj);

            const auto SBi = sys.adjust_direction(rBi - rSi); // Si -> Bi
            const auto SBj = sys.adjust_direction(rBj - rSj); // Sj -> Bj

            const auto lSBi_sq = math::length_sq(SBi); // |SBi|^2
            const auto lSBj_sq = math::length_sq(SBj); // |SBj|^2

            const auto rlSBi = math::rsqrt(lSBi_sq); // 1 / |SBi|
            const auto rlSBj = math::rsqrt(lSBi_sq); // 1 / |SBj|

            const auto dot_SBiBj = -math::dot_product(SBi, Bij);
            const auto dot_SBjBi =  math::dot_product(SBj, Bij);

            const auto cos_theta1 = dot_SBiBj * rlSBi * rlBij;
            const auto cos_theta2 = dot_SBjBi * rlSBj * rlBij;

            const auto theta1 = std::acos(math::clamp<real_type>(cos_theta1, -1, 1));
            const auto theta2 = std::acos(math::clamp<real_type>(cos_theta2, -1, 1));

            // ----------------------------------------------------------------
            // The second term of base-pairing
            //  = 1/2(1+cos(dphi)) f(dtheta1) f(dtheta2) U_attr(rij)
            //
            // f(theta) = 1                             ... abs(dtheta) < pi/2K
            //            1 - cos^2(K (theta - theta0)) ... pi/2K < abs(dtheta) < pi/K
            //            0                             ... pi/K  < abs(dtheta)

            const auto f1 = base_pairing_potential_.f(bp_kind, theta1);
            const auto f2 = base_pairing_potential_.f(bp_kind, theta2);

            if(f1 != real_type(0.0) && f2 != real_type(0.0)) // [[likely]]
            {
                // if both fs are non-zero, the attractive part of base-pairing
                // has a non-zero value. calculate dihedral and cos(dphi).

                // ------------------------------------------------------------
                //  Si o         o Si
                //      \       /
                //    Bi o =(= o Bj
                //         phi

                const auto Bij_reg = Bij * rlBij;
                const auto R = math::dot_product(SBi, Bij_reg) * Bij_reg - SBi;
                const auto S = math::dot_product(SBj, Bij_reg) * Bij_reg - SBj;

                const auto dot_RS  = math::dot_product(R, S) *
                                     math::rsqrt(R_lensq * S_lensq);
                const auto cos_phi = math::clamp<real_type>(dot_RS, -1, 1);

                const auto n    =  math::cross_product(Bij, SBj);
                const auto sign = -math::dot_product(SBi, n);
                const auto phi  = std::copysign(std::acos(cos_phi), sign);

                auto dphi = phi - base_pairing_potential_.phi_0(bp_kind);
                if     (pi   < dphi) {dphi -= two_pi;}
                else if(dphi <  -pi) {dphi += two_pi;}

                const auto cos_dphi = std::cos(dphi);

                // ------------------------------------------------------------
                // U_attr = -e_ij                                       .. r < r0
                //          -e_ij + e_ij(1 - exp(-a_ij(rij - r0_ij)))^2 .. r0 < r

                real_type U_attr = -e_BP;
                if(r0 < lBij)
                {
                    const auto term = real_type(1.0) - std::exp(-a_BP * (lBij - r0));
                    U_attr += e_BP * term * term;
                }

                // ------------------------------------------------------------
                // The second term of base-pairing
                //  = 1/2(1 + cos(dphi)) f(dtheta1) f(dtheta2) U_attr(rij)
                E += 0.5 * (1 + cos_dphi) * f1 * f2 * U_attr;
            }

            // ----------------------------------------------------------------
            // cross stacking
            // f(theta_3) f(theta_CS) U_attr(epsilon, alpha, rij)
            //
            //       Si   Bi   Bj   Sj
            //  5'    o -- o===o -- o     3'
            //  ^    /      \ /      \    |
            //  | P o        x        o P |
            //  |    \      / \      /    v
            //  3'    o -- o===o -- o     5'
            //           Bi3   Bj5

            const auto Bi3 = cross_stacking_potential_.parameters()[Bi].B3;
            const auto Bj5 = cross_stacking_potential_.parameters()[Bj].B5;

            const bool Bi3_exists = (Bi3 != cross_stacking_potential_.invalid());
            const bool Bj5_exists = (Bj5 != cross_stacking_potential_.invalid());

            if(!Bi3_exists && !Bj5_exists)
            {
                continue; // if both interacting pair do not exist, do nothing.
            }

            const auto dot_SBiSBj = math::dor_product(SBi, SBj);
            const auto cos_theta3 = dot_SBiSBj * rlSBi * rlSBj;
            const auto theta3     = std::acos(math::clamp<real_type>(cos_theta3, -1, 1));
            const auto f3         = cross_stacking_potential_.f(bp_kind, theta3);
            if(f3 == real_type(0))
            {
                // both cross-stacking becomes zero. skip them.
                continue;
            }

            // 5' adjacent of Base j might not exist, e.g., edge of the DNA.
            if(Bj5_exists)
            {
                // ----------------------------------------------------------------
                // cross stacking
                // f(theta_3) f(theta_CS) U_attr(epsilon, alpha, rij)
                //
                //       Si   Bi   Bj   Sj
                //  5'    o--> o===o <--o     3'
                //  ^    /   `--\        \    |
                //  | P o   tCS  \        o P |
                //  |    \        \      /    v
                //  3'    o -- o===o -- o     5'
                //           Bi3   Bj5

                const auto cs_kind = cross_stacking_potential_.cs5_kind(Bi, Bj5);
                const auto& rBj5 = sys.position(Bj5);

                const auto Bj5i     = sys.adjust_direction(rBi - rBj5);
                const auto lBj5i_sq = math::length_sq(lBj5i);
                const auto rlBj5i   = math::rsqrt(lBj5i_sq);

                const auto dot_theta_CS = math::dot_product(SBi, Bj5i);
                const auto cos_theta_CS = dot_theta_CS * rlSBi * rlBj5i;
                const auto theta_CS     =
                    std::acos(math::clamp<real_type>(cos_theta_CS, -1, 1));

                const auto fCS = cross_stacking_potential_.f(cs_kind, theta_CS);
                if(fCS != real_type(0))
                {
                    const auto e_CS  = cross_stacking_potential_.epsilon(cs_kind);
                    const auto a_CS  = cross_stacking_potential_.alpha();
                    const auto r0_CS = cross_stacking_potential_.r0(cs_kind);
                    const auto lBj5i = lBj5i_sq * rlBj5i;

                    // --------------------------------------------------------
                    // U_attr =
                    //   -e_ij                                       .. r < r0
                    //   -e_ij + e_ij(1 - exp(-a_ij(rij - r0_ij)))^2 .. r0 < r

                    real_type U_attr = -e_CS;
                    if(r0 < lBij)
                    {
                        const auto term = real_type(1.0) -
                                          std::exp(-a_CS * (lBj5i - r0_CS));
                        U_attr += e_CS * term * term;
                    }

                    E += f3 * fCS * U_attr;
                }
            }

            // 3' adjacent of Base i might not exist, e.g., edge of the DNA.
            if(Bi3_exists)
            {
                // ----------------------------------------------------------------
                // cross stacking
                // f(theta_3) f(theta_CS) U_attr(epsilon, alpha, rij)
                //
                //       Si   Bi   Bj   Sj
                //  5'    o--> o===o <--o     3'
                //  ^    /        /--'   \    |
                //  | P o        /  tCS   o P |
                //  |    \      /        /    v
                //  3'    o -- o===o -- o     5'
                //           Bi3   Bj5

                const auto cs_kind = cross_stacking_potential_.cs3_kind(Bj, Bi3);
                const auto& rBi3   = sys.position(Bi3);

                const auto Bi3j     = sys.adjust_direction(rBj - rBi3);
                const auto lBi3j_sq = math::length_sq(lBi3j);
                const auto rlBi3j   = math::rsqrt(lBi3j_sq);

                const auto dot_theta_CS = math::dot_product(SBj, Bi3j);
                const auto cos_theta_CS = dot_theta_CS * rlSBj * rlBi3j;
                const auto theta_CS     =
                    std::acos(math::clamp<real_type>(cos_theta_CS, -1, 1));

                const auto fCS = cross_stacking_potential_.f(cs_kind, theta_CS);
                if(fCS != real_type(0))
                {
                    const auto e_CS  = cross_stacking_potential_.epsilon(cs_kind);
                    const auto a_CS  = cross_stacking_potential_.alpha();
                    const auto r0_CS = cross_stacking_potential_.r0(cs_kind);
                    const auto lBi3j = lBi3j_sq * rlBi3j;

                    // --------------------------------------------------------
                    // U_attr =
                    //   -e_ij                                       .. r < r0
                    //   -e_ij + e_ij(1 - exp(-a_ij(rij - r0_ij)))^2 .. r0 < r

                    real_type U_attr = -e_CS;
                    if(r0 < lBij)
                    {
                        const auto term = real_type(1.0) -
                                          std::exp(-a_CS * (lBi3j - r0_CS));
                        U_attr += e_CS * term * term;
                    }
                    E += f3 * fCS * U_attr;
                }
            }
        }
    }
    return E;
}

} // mjolnir
#endif // MJOLNIR_INTERACTION_GLOBAL_3SPN_BASE_PAIRING_INTERACTION_HPP
