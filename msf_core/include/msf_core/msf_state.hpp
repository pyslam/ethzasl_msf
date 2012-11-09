/*
 *  Created on: Nov 7, 2012
 *      Author: slynen
 */



#ifndef MSF_STATE_HPP_
#define MSF_STATE_HPP_

#include <msf_core/msf_types.tpp>
#include <msf_core/msf_tmp.hpp>
#include <msf_core/msf_statedef.hpp>
#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <vector>
#include <msf_core/eigen_conversions.h>
#include <sensor_fusion_comm/ExtState.h>
#include <sensor_fusion_comm/DoubleArrayStamped.h>
#include <geometry_msgs/PoseWithCovarianceStamped.h>

namespace msf_core{

//a state variable with a name as specified in the state name enum
template<typename type_T, int name_T, int STATETYPE>
struct StateVar_T{
	typedef type_T value_t;
	typedef const StateVar_T<type_T, name_T>& constRef_T;
	typedef const StateVar_T<type_T, name_T>* constPtr_T;
	typedef StateVar_T<type_T, name_T>& Ref_T;
	typedef StateVar_T<type_T, name_T>* Ptr_T;
	enum{
		statetype_ = STATETYPE,
		name_ = name_T,
		sizeInCorrection_ = msf_tmp::CorrectionStateLengthForType<const StateVar_T<type_T, name_T>&>::value,
		sizeInState_ = msf_tmp::StateLengthForType<const StateVar_T<type_T, name_T>&>::value
	};
	typedef Eigen::Matrix<double, sizeInCorrection_, sizeInCorrection_> Q_T;
	Q_T Q_;
	value_t state_;
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};


template<typename stateSequence_T>
struct GenericState_T{
	friend class msf_core::MSF_Core;
	friend class msf_core::copyNonPropagationStates<GenericState_T>;
public:
	typedef stateSequence_T stateVector_T;
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
	enum{
		nStateVarsAtCompileTime = boost::fusion::result_of::size<stateVector_T>::type::value, //n state vars
		nErrorStatesAtCompileTime = msf_tmp::CountStates<stateVector_T, msf_tmp::CorrectionStateLengthForType>::value, //n error states
		nStatesAtCompileTime = msf_tmp::CountStates<stateVector_T, msf_tmp::StateLengthForType>::value, //n total states
		nCoreStatesAtCompileTime = msf_tmp::CountStates<stateVector_T, msf_tmp::CoreStateLengthForType>::value, //n total core states
		nPropagatedCoreStatesAtCompileTime = msf_tmp::CountStates<stateVector_T, msf_tmp::PropagatedCoreStateLengthForType>::value, //n total core states with propagation
		nPropagatedCoreErrorStatesAtCompileTime = msf_tmp::CountStates<stateVector_T, msf_tmp::PropagatedCoreErrorStateLengthForType>::value //n total error states with propagation
	};

private:

	//returns the stateVar at position INDEX in the state list, non const version only for msf_core use
	//you must not make these functions public. Instead const_cast the state object to const to use the overload
	template<int INDEX>
	inline typename boost::fusion::result_of::at_c<stateVector_T, INDEX >::type
	getStateVar();

	//returns the state at position INDEX in the state list, non const version
	//you must not make these functions public. Instead const_cast the state object to const to use the overload
	template<int INDEX>
	inline typename msf_tmp::StripReference<typename boost::fusion::result_of::at_c<stateVector_T, INDEX >::type>::result_t::value_t&
	get();

public:

	stateVector_T statevars_; ///< the actual state variables

	// system inputs
	Eigen::Matrix<double,3,1> w_m_;         ///< angular velocity from IMU
	Eigen::Matrix<double,3,1> a_m_;         ///< acceleration from IMU

	Eigen::Quaternion<double> q_int_;       ///< this is the integrated ang. vel. no corrections applied, to use for delta rot in external algos...

	double time_; 				///< time of this state estimate
	Eigen::Matrix<double, nErrorStatesAtCompileTime, nErrorStatesAtCompileTime> P_;///< error state covariance

	//apply the correction vector to all state vars
	inline void correct(const Eigen::Matrix<double, nErrorStatesAtCompileTime, 1>& correction);

	//returns the Q-block of the state at position INDEX in the state list, not allowed for core states
	template<int INDEX>
	inline typename msf_tmp::StripReference<typename boost::fusion::result_of::at_c<stateVector_T, INDEX >::type>::result_t::Q_T&
	getQBlock();

	//returns the Q-block of the state at position INDEX in the state list, also possible for core states, since const
	template<int INDEX>
	inline const typename msf_tmp::StripReference<typename boost::fusion::result_of::at_c<stateVector_T, INDEX >::type>::result_t::Q_T&
	getQBlock() const;

	/// resets the state
	/**
	 * 3D vectors: 0; quaternion: unit quaternion; scale: 1; time:0; Error covariance: zeros
	 */
	void reset(boost::shared_ptr<UserDefinedCalculationBase> usercalc);

	/// writes the covariance corresponding to position and attitude to cov
	void getPoseCovariance(geometry_msgs::PoseWithCovariance::_covariance_type & cov); //boost fusion unfortunately doesn't like this to be const

	/// assembles a PoseWithCovarianceStamped message from the state
	/** it does not set the header */
	void toPoseMsg(geometry_msgs::PoseWithCovarianceStamped & pose);

	/// assembles an ExtState message from the state
	/** it does not set the header */
	void toExtStateMsg(sensor_fusion_comm::ExtState & state);

	/// assembles a DoubleArrayStamped message from the state
	/** it does not set the header */
	void toFullStateMsg(sensor_fusion_comm::DoubleArrayStamped & state);

	/// assembles a DoubleArrayStamped message from the state
	/** it does not set the header */
	void toCoreStateMsg(sensor_fusion_comm::DoubleArrayStamped & state);


	///returns the state at position INDEX in the state list, const version
	template<int INDEX>
	inline const typename msf_tmp::StripReference<typename boost::fusion::result_of::at_c<stateVector_T, INDEX >::type>::result_t::value_t&
	get() const;

	///returns the stateVar at position INDEX in the state list, const version
	template<int INDEX>
	inline typename msf_tmp::AddConstReference<typename boost::fusion::result_of::at_c<stateVector_T, INDEX >::type>::result_t
	getStateVar() const;

	///sets state at position INDEX in the state list, fails for core states at compile time
	template<int INDEX>
	inline void
	set(const typename msf_tmp::StripConstReference<typename boost::fusion::result_of::at_c<stateVector_T, INDEX >::type>::result_t::value_t& newvalue);
};

}

#include <msf_core/implementation/msf_state_.hpp>

#endif /* MSF_STATE_HPP_ */
