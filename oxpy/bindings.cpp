/*
 * bindings.cpp
 *
 *  Created on: Sep 13, 2019
 *      Author: lorenzo
 */

#include "python_defs.h"

#include "OxpyContext.h"
#include "OxpyManager.h"

#include <Interactions/BaseInteraction.h>
#include <Observables/BaseObservable.h>
#include <Particles/BaseParticle.h>
#include <Utilities/ConfigInfo.h>
#include "vector_matrix_casters.h"

void export_BaseObservable(py::module &m);
void export_BaseParticle(py::module &m);
void export_ConfigInfo(py::module &m);
void export_IBaseInteraction(py::module &m);

PYBIND11_MODULE(core, m) {
	export_OxpyContext(m);

	export_SimManager(m);
	export_OxpyManager(m);

	export_BaseObservable(m);
	export_BaseParticle(m);
	export_ConfigInfo(m);
	export_IBaseInteraction(m);
}

// trampoline class for BaseObservable
class PyBaseObservable : public BaseObservable {
public:
	using BaseObservable::BaseObservable;

	std::string get_output_string(llint curr_step) override {
		PYBIND11_OVERLOAD_PURE( // @suppress("Unused return value")
				std::string,
				BaseObservable,
				get_output_string,
				curr_step
		);
	}
};

void export_BaseObservable(py::module &m) {
	py::class_<BaseObservable, PyBaseObservable, std::shared_ptr<BaseObservable>> obs(m, "BaseObservable", R"pbdoc(
		The interface class for observables.
	)pbdoc");

	obs.def(py::init<>(), R"pbdoc(
        The default constructor takes no parameters.
	)pbdoc");

	obs.def("get_settings", &BaseObservable::get_settings, py::arg("my_inp"), py::arg("sim_inp"), R"pbdoc(
		Computes the quantity/quantities of interest and returns the output string.

		Parameters
		---------- 
		my_inp: :class:`input_file`
			The input file of the observable.
		sim_inp: :class:`input_file`
			The general input file of the simulation.
	)pbdoc");

	obs.def("init", &BaseObservable::init, py::arg("config_info"), R"pbdoc(
		Initialises the observable.

		Parameters
		---------- 
		config_info: :class:`ConfigInfo`
			The singleton object storing the simulation details.
	)pbdoc");

	obs.def("get_output_string", &BaseObservable::get_output_string, py::arg("curr_step"), R"pbdoc(
		Computes the quantity/quantities of interest and returns the output string.

        Parameters
        ---------- 
        curr_step: int
            The current simulation step.

        Returns
        -------
        str
            The output of the observable.
	)pbdoc");
}

void export_BaseParticle(py::module &m) {
	py::class_<BaseParticle, std::shared_ptr<BaseParticle>> particle(m, "BaseParticle", R"pbdoc(
        A simulation particle.
	)pbdoc");

	particle.def(py::init<>(), R"pbdoc(
		 The default constructor takes no parameters.
	)pbdoc");
	particle.def("is_bonded", &BaseParticle::is_bonded, py::arg("q"), R"pbdoc(
        Return whether the current particle and q are bonded neighbours.

        Parameters
        ----------
        q: :class:`BaseParticle`
            The other Particle.

        Returns
        -------
        bool
            True if the current particle and :attr:`q` are bonded neighbours.
    )pbdoc");
	particle.def_readwrite("index", &BaseParticle::index, R"pbdoc(
        The index of the particle.
    )pbdoc");
	particle.def_readwrite("type", &BaseParticle::type, R"pbdoc(
        The type of the particle.
	)pbdoc");
	particle.def_readwrite("btype", &BaseParticle::btype, R"pbdoc(
		The btype of the particle.
	)pbdoc");
	particle.def_readwrite("strand_id", &BaseParticle::strand_id, R"pbdoc(
		The id of the strand to which the particle belongs.
	)pbdoc");
	particle.def_readwrite("pos", &BaseParticle::pos, R"pbdoc(
		The position of the particle.
	)pbdoc");
	particle.def_readwrite("orientation", &BaseParticle::orientation, R"pbdoc(
		The orientation of the particle as a 3x3 matrix.
	)pbdoc");
	particle.def_readwrite("vel", &BaseParticle::vel, R"pbdoc(
		The velocity of the particle.
	)pbdoc");
	particle.def_readwrite("L", &BaseParticle::L, R"pbdoc(
		The angular momentum of the particle.
	)pbdoc");
	particle.def_readwrite("force", &BaseParticle::force, R"pbdoc(
		The force exerted on the particle.
	)pbdoc");
	particle.def_readwrite("torque", &BaseParticle::torque, R"pbdoc(
		The torque exerted on the particle.
	)pbdoc");
	particle.def_readwrite("ext_potential", &BaseParticle::ext_potential, R"pbdoc(
		The potential energy due to the external forces acting on the particle.
	)pbdoc");
	particle.def_readwrite("n3", &BaseParticle::n3, py::return_value_policy::reference, R"pbdoc(
		The n3 neighbour.
	)pbdoc");
	particle.def_readwrite("n5", &BaseParticle::n5, py::return_value_policy::reference, R"pbdoc(
		The n5 neighbour.
	)pbdoc");
}

void export_ConfigInfo(py::module &m) {
	py::class_<ConfigInfo, std::shared_ptr<ConfigInfo>> conf_info(m, "ConfigInfo", R"pbdoc(
		 This singleton object stores all the details of the simulation (particles, neighbour lists, input file, interaction) 
	)pbdoc");

	conf_info.def("N", &ConfigInfo::N, R"pbdoc(
         Return the current number of particles.

         Returns
         -------
         int
             The number of particles in the simulation box.
	)pbdoc");
	conf_info.def("particles", &ConfigInfo::particles, py::return_value_policy::reference, R"pbdoc(
		 Return a list of all the particles.

         Returns
         -------
         List(:py:class:`BaseParticle`)
             A list containing all the particles in the simulation box.
	)pbdoc");
	conf_info.def_readwrite("interaction", &ConfigInfo::interaction, R"pbdoc(
		 The simulation's :py:class:`IBaseInteraction` object.
	)pbdoc");
}

// trampoline class for IBaseInteraction
class PyIBaseInteraction : public IBaseInteraction {
public:
	using IBaseInteraction::IBaseInteraction;

	void init() override {
		PYBIND11_OVERLOAD_PURE( // @suppress("Unused return value")
				void,
				IBaseInteraction,
				init
		);
	}

	void allocate_particles(std::vector<BaseParticle *> &particles) override {
		PYBIND11_OVERLOAD_PURE( // @suppress("Unused return value")
				void,
				IBaseInteraction,
				allocate_particles,
				particles
		);
	}

	void check_input_sanity(std::vector<BaseParticle *> &particles) override {
		PYBIND11_OVERLOAD_PURE( // @suppress("Unused return value")
				void,
				IBaseInteraction,
				check_input_sanity,
				particles
		);
	}

	number pair_interaction(BaseParticle *p, BaseParticle *q, bool compute_r = true, bool update_forces = false) override {
		PYBIND11_OVERLOAD_PURE( // @suppress("Unused return value")
				number,
				IBaseInteraction,
				pair_interaction,
				p,
				q,
				compute_r,
				update_forces
		);

		// suppress warnings
		return 0.;
	}

	number pair_interaction_bonded(BaseParticle *p, BaseParticle *q, bool compute_r = true, bool update_forces = false) override {
		PYBIND11_OVERLOAD_PURE( // @suppress("Unused return value")
				number,
				IBaseInteraction,
				pair_interaction_bonded,
				p,
				q,
				compute_r,
				update_forces
		);

		// suppress warnings
		return 0.;
	}

	number pair_interaction_nonbonded(BaseParticle *p, BaseParticle *q, bool compute_r = true, bool update_forces = false) override {
		PYBIND11_OVERLOAD_PURE( // @suppress("Unused return value")
				number,
				IBaseInteraction,
				pair_interaction_nonbonded,
				p,
				q,
				compute_r,
				update_forces
		);

		// suppress warnings
		return 0.;
	}

	number pair_interaction_term(int name, BaseParticle *p, BaseParticle *q, bool compute_r = true, bool update_forces = false) override {
		PYBIND11_OVERLOAD_PURE( // @suppress("Unused return value")
				number,
				IBaseInteraction,
				pair_interaction_term,
				name,
				p,
				q,
				compute_r,
				update_forces
		);

		// suppress warnings
		return 0.;
	}

	std::map<int, number> get_system_energy_split(std::vector<BaseParticle *> &particles, BaseList *lists) override {
		using ret_type = std::map<int, number>;
		PYBIND11_OVERLOAD_PURE( // @suppress("Unused return value")
				ret_type,
				IBaseInteraction,
				get_system_energy_split,
				particles,
				lists
		);

		// suppress warnings
		return std::map<int, number>();
	}
};

void export_IBaseInteraction(py::module &m) {
	py::class_<IBaseInteraction, PyIBaseInteraction, std::shared_ptr<IBaseInteraction>> interaction(m, "IBaseInteraction", R"pbdoc(
		The class that takes care of computing the interaction between the particles.
	)pbdoc");

	interaction.def(py::init<>(), R"pbdoc(
		The default constructor takes no parameters. 
	)pbdoc");
	interaction.def("set_computed_r", &IBaseInteraction::set_computed_r, py::arg("r"), R"pbdoc(
        Set the distance vector used by the `pair_interaction_*` methods when they are called with `compute_r = False` (see :meth:`pair_interaction` for additional details).

        Parameters
        ----------
        r : numpy.ndarray
            The distance vector to be stored.
	)pbdoc");
	interaction.def("pair_interaction", &IBaseInteraction::pair_interaction, py::arg("p"), py::arg("q"), py::arg("compute_r") = true, py::arg("update_forces") = false, R"pbdoc(
        Compute the pair interaction between p and q.

        Parameters
        ----------
        p : :class:`BaseParticle`
            The first particle of the pair. Note that some interactions require that the two particles are passed to the method with a specific order.
        q : :class:`BaseParticle`
            The second particle of the pair.
        compute_r : bool
            If True (default value), the distance between :attr:`p` and :attr:`q` will be computed from scratch. If not, it will use a private member that can be
            set through the :meth:`set_computed_r` method.
        update_forces : bool
            If True, the forces and torques acting on the two particles will be updated (defaults to False).

        Returns
        -------
        float
            The energy of the pair interaction.
    )pbdoc");
	interaction.def("pair_interaction_bonded", &IBaseInteraction::pair_interaction_bonded, py::arg("p"), py::arg("q"), py::arg("compute_r") = true, py::arg("update_forces") = false, R"pbdoc(
        Compute the bonded pair interaction between p and q. See :meth:`pair_interaction` for details on the parameters and on the return value.
	)pbdoc");
	interaction.def("pair_interaction_nonbonded", &IBaseInteraction::pair_interaction_nonbonded, py::arg("p"), py::arg("q"), py::arg("compute_r") = true, py::arg("update_forces") = false, R"pbdoc(
        Compute the unbonded pair interaction between p and q. See :meth:`pair_interaction` for details on the parameters and on the return value.
	)pbdoc");
}
