/**
 * @file    Depletion.cpp
 * @date    30/ott/2018
 * @author  flavio
 *
 */
#include "Depletion.h"

#define DEPLETION_TRANSLATION (0)
#define DEPLETION_ROTATION (1)
#define DEPLETION_SWIM (2)

template<typename number>
Depletion<number>::Depletion (){
	_delta_trs = (number) -1.f;
	_delta_rot = (number) -1.f;
	_delta_swm = (number) -1.f;
	_delta_trs_max = (number) -1.f;
	_delta_rot_max = (number) -1.f;
	_delta_swm_max = (number) -1.f;
	_ntries = -1;
	_sigma_dep = 0.5f;
	_rho_dep = -1.f;
	_tryvolume = -1.f;
	_mu_gas = 1.;
}

template<typename number>
Depletion<number>::~Depletion () {

}

template<typename number>
void Depletion<number>::init () {
	BaseMove<number>::init();
	_tryvolume = (10. + 2. * _sigma_dep) * pow((0.5 + _sigma_dep), 2) * M_PI;
	_ntries = (int)(exp(_mu_gas / this->_T) * _tryvolume);
	if (_ntries < 1 || _ntries > 50) {
		OX_LOG(Logger::LOG_WARNING, "(Depletion.cpp) Too large ntries. Setting it to 50");
		_ntries = 50;
	}
	if (this->_restrict_to_type < 0) throw oxDNAException("Depletion move MUST be restricted to a type");
	OX_LOG(Logger::LOG_INFO, "(Depletion.cpp) Depletion move initiated with delta_trs=%g, delta_trs_max=%g, delta_rot=%g, delta_rot_max=%g, delta_swm=%g, delta_swm_max=%g", _delta_trs, _delta_trs_max, _delta_rot, _delta_rot_max, _delta_swm, _delta_swm_max);
	OX_LOG(Logger::LOG_INFO, "(Depletion.cpp)                               tries=%d, sigma_dep=%g, mu_gas=%g, tryvolume=%g", _ntries, _sigma_dep, _mu_gas, _tryvolume);
	OX_LOG(Logger::LOG_INFO, "(Depletion.cpp)                               restrict_to_type=%d, and probability %g", this->_restrict_to_type, this->prob);
}

template<typename number>
void Depletion<number>::get_settings (input_file &inp, input_file &sim_inp) {
	BaseMove<number>::get_settings (inp, sim_inp);

	double tmpf[3];
	std::string tmpstr;
	getInputString (&inp, "deltas", tmpstr, 1);
	int tmpi = sscanf(tmpstr.c_str(), "%lf,%lf,%lf", tmpf, tmpf + 1, tmpf + 2);
	_delta_trs = (number) tmpf[0];
	_delta_rot = (number) tmpf[1];
	_delta_swm = (number) tmpf[2];
	if(tmpi != 3) throw oxDNAException ("(Depletion.cpp) Could not parse deltas (found deltas=%s, provide deltas=<float>,<float>,<float>)", tmpstr.c_str());
	getInputString (&inp, "deltas_max", tmpstr, 1);
	tmpi = sscanf(tmpstr.c_str(), "%lf,%lf,%lf", tmpf, tmpf + 1, tmpf + 2);
	if(tmpi != 3) throw oxDNAException ("(Depletion.cpp) Could not parse deltas (found deltas_max=%s, provide deltas_max=<float>,<float>,<float>)", tmpstr.c_str());
	_delta_trs_max = (number) tmpf[0];
	_delta_rot_max = (number) tmpf[1];
	_delta_swm_max = (number) tmpf[2];
	getInputNumber (&inp, "sigma_dep", &_sigma_dep, 1);
	//getInputNumber (&inp, "rho_dep", &_rho_dep, 1);
	getInputNumber (&inp, "mu_gas", &_mu_gas, 1);
	getInputNumber (&inp, "tryvolume", &_tryvolume, 1);
	getInputInt (&inp, "ntries", &_ntries, 1);
}

template<typename number>
void Depletion<number>::apply (llint curr_step) {

	number length = 10.;

	// we increase the attempted count
	this->_attempted += 1;

	// we select the particle to translate
	int pi = (int) (drand48() * (*this->_Info->N));
	BaseParticle<number> * p = this->_Info->particles[pi];
	if (this->_restrict_to_type >= 0) {
		while(p->type != this->_restrict_to_type) {
			pi = (int) (drand48() * (*this->_Info->N));
			p = this->_Info->particles[pi];
		}
	}

	// compute the energy before the move
	number delta_E;
	if (this->_compute_energy_before) delta_E = -this->particle_energy(p);
	else delta_E = (number) 0.f;
	p->set_ext_potential (curr_step, this->_Info->box);
	number delta_E_ext = -p->ext_potential;

	std::vector<BaseParticle<number> *> neighs_old = this->_Info->lists->get_complete_neigh_list(p);

	// move_particle
	int move_type;
	int choice = lrand48() % 21;
	if ( 0 <= choice && choice < 10) move_type = DEPLETION_TRANSLATION;
	if (10 <= choice && choice < 20) move_type = DEPLETION_ROTATION;
	if (20 <= choice && choice < 21) move_type = DEPLETION_SWIM;

	LR_vector<number> pos_old = p->pos;
	LR_matrix<number> orientation_old = p->orientation;
	LR_matrix<number> orientationT_old = p->orientationT;
	if (move_type == DEPLETION_TRANSLATION) {
		p->pos.x += (number) 2.f * _delta_trs * (drand48() - 0.5);
		p->pos.y += (number) 2.f * _delta_trs * (drand48() - 0.5);
		p->pos.z += (number) 2.f * _delta_trs * (drand48() - 0.5);
	}
	if (move_type == DEPLETION_ROTATION) {
		LR_matrix<number> R = Utils::get_random_rotation_matrix_from_angle<number> (_delta_rot * drand48());
		p->orientation = R * p->orientation;
		p->orientationT = p->orientation.get_transpose();
		p->set_positions();
	}
	if (move_type == DEPLETION_SWIM) {
		p->pos += ((number) 2.f * _delta_swm * (number)(drand48() - 0.5)) * p->orientation.v3;
	}

	this->_Info->lists->single_update(p);
	if(!this->_Info->lists->is_updated()) {
		this->_Info->lists->global_update();
	}

	// see if there is an overlap
	std::vector<BaseParticle<number> *> neighs_new = this->_Info->lists->get_complete_neigh_list(p);
	typename std::vector<BaseParticle<number> *>::iterator it;

	// energy after the move
	delta_E += this->particle_energy(p);
	p->set_ext_potential(curr_step, this->_Info->box);
	delta_E_ext += p->ext_potential;

	BaseParticle<number> * p_new = p;
	BaseParticle<number> * p_old = new BaseParticle<number> ();
	p_old->type = p->type;
	p_old->pos = pos_old;
	p_old->orientation = orientation_old;
	p_old->orientationT = orientationT_old;
	p_old->index = p->index;

	// helper particle
	BaseParticle<number> * q = new BaseParticle<number> ();
	q->type = this->_restrict_to_type + 1;

	//printf ("%d\n", _ntries);

	// if there is no overlap between large particles, we handle the depletant
	if (this->_Info->interaction->get_is_infinite() == false) {

		// for each depletant we test if it overlaps
		//int n_dep = _rho_dep * _tryvolume; // put poissonian here

		// free volume before the move;
		int ntry = 0, nfv = 0;
		while (ntry < _ntries) {
			number dx = 2.*drand48() - 1.; // between -1 and 1
			number dy = 2.*drand48() - 1.; // between -1 and 1
			while (dx*dx + dy*dy >= 1.) {
				dx = 2.*drand48() - 1.;
				dy = 2.*drand48() - 1.;
			}
			q->pos = p_new->pos + p_new->orientation.v1 * (0.5 + _sigma_dep) * dx +
			                      p_new->orientation.v2 * (0.5 + _sigma_dep) * dy +
								  p_new->orientation.v3 * (length + 2. * _sigma_dep) * (drand48() - 0.5);

			// we check if we overlap with any of the other particles, p_old included
			number junk = this->_Info->interaction->pair_interaction(p_old, q);
			for (it = neighs_new.begin(); it != neighs_new.end(); ++it) {
				junk += this->_Info->interaction->pair_interaction(*it, q);
				if ((*it)->type != this->_restrict_to_type) throw oxDNAException("disaster %d", __LINE__);
				if (this->_Info->interaction->get_is_infinite() == true) break;
			}

			if (this->_Info->interaction->get_is_infinite() == false) nfv ++;
			this->_Info->interaction->set_is_infinite(false);

			ntry ++;
		}

		number fv_old = _tryvolume * nfv / (number) _ntries;

		ntry = 0;
		nfv = 0;
		while (ntry < _ntries) {
			number dx = 2.*drand48() - 1.; // between -1 and 1
			number dy = 2.*drand48() - 1.; // between -1 and 1
			while (dx*dx + dy*dy >= 1.) {
				dx = 2.*drand48() - 1.;
				dy = 2.*drand48() - 1.;
			}
			q->pos = p_old->pos + p_old->orientation.v1 * (0.5 + _sigma_dep) * dx +
								  p_old->orientation.v2 * (0.5 + _sigma_dep) * dy +
								  p_old->orientation.v3 * (length + 2. * _sigma_dep) * (drand48() - 0.5);

			// we check if we overlap with any of the other particles, p_old included
			number junk = this->_Info->interaction->pair_interaction(p_new, q);
			for (it = neighs_old.begin(); it != neighs_old.end(); ++it) {
				junk += this->_Info->interaction->pair_interaction(*it, q);
				if ((*it)->type != this->_restrict_to_type) throw oxDNAException("disaster %d", __LINE__);
				if (this->_Info->interaction->get_is_infinite() == true) break;
			}

			if (this->_Info->interaction->get_is_infinite() == false) nfv ++;
			this->_Info->interaction->set_is_infinite(false);

			ntry ++;
		}
		number fv_new = _tryvolume * nfv / (number) _ntries;

		delta_E += - _mu_gas * (fv_new  - fv_old);
		//printf ("%g %g %g %g %d %d\n", delta_E, fv_new - fv_old, fv_new, fv_old, nfv, nfvold);
	}

	delete q;
	delete p_old;


	// accept or reject?
	if (this->_Info->interaction->get_is_infinite() == false &&
		((delta_E + delta_E_ext) < 0 || exp(-(delta_E + delta_E_ext) / this->_T) > drand48()) ) {
		// move accepted
        this->_accepted ++;

		if (curr_step < this->_equilibration_steps && this->_adjust_moves) {
			if (move_type == DEPLETION_TRANSLATION) {
				_delta_trs *= this->_acc_fact;
				if (_delta_trs >= _delta_trs_max) _delta_trs = _delta_trs_max;
			}
			if (move_type == DEPLETION_ROTATION) {
				_delta_rot *= this->_acc_fact;
				if (_delta_rot >= _delta_rot_max) _delta_rot = _delta_rot_max;
			}
			if (move_type == DEPLETION_SWIM) {
				_delta_swm *= this->_acc_fact;
				if (_delta_swm >= _delta_swm_max) _delta_swm = _delta_swm_max;
			}
		}
	}
	else {
		if (move_type == DEPLETION_TRANSLATION || move_type == DEPLETION_SWIM) {
			p->pos = pos_old;
		}
		else {
			p->orientation = orientation_old;
			p->orientationT = orientationT_old;
			p->set_positions();
		}

		this->_Info->lists->single_update(p);
		this->_Info->interaction->set_is_infinite(false);
		if(!this->_Info->lists->is_updated()) {
			this->_Info->lists->global_update();
		}

		if (curr_step < this->_equilibration_steps && this->_adjust_moves) {
			if (move_type == DEPLETION_TRANSLATION) _delta_trs /= this->_rej_fact;
			if (move_type == DEPLETION_ROTATION) _delta_rot /= this->_rej_fact;
			if (move_type == DEPLETION_SWIM) _delta_rot /= this->_rej_fact;
		}
	}

	return;
}

template<typename number>
void Depletion<number>::log_parameters() {
	BaseMove<number>::log_parameters();
	OX_LOG(Logger::LOG_INFO, "\tdelta_trs = %g, delta_rot = %g, delta_swm = %g", _delta_trs, _delta_rot, _delta_swm);
}

template class Depletion<float>;
template class Depletion<double>;