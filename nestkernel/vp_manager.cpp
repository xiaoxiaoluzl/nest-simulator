/*
 *  vp_manager.cpp
 *
 *  This file is part of NEST.
 *
 *  Copyright (C) 2004 The NEST Initiative
 *
 *  NEST is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  NEST is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with NEST.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "vp_manager.h"

// Includes from libnestutil:
#include "logging.h"

// Includes from nestkernel:
#include "kernel_manager.h"
#include "mpi_manager.h"
#include "mpi_manager_impl.h"
#include "vp_manager_impl.h"

// Includes from sli:
#include "dictutils.h"

nest::VPManager::VPManager()
#ifdef _OPENMP
  : force_singlethreading_( false )
#else
  : force_singlethreading_( true )
#endif
  , n_threads_( 1 )
{
}

void
nest::VPManager::initialize()
{
// When the VPManager is initialized, you will have 1 thread again.
// Setting more threads will be done via nest::set_kernel_status
#ifdef _OPENMP
  /* The next line is required because we use the OpenMP
   threadprivate() directive in the allocator, see OpenMP
   API Specifications v 3.1, Ch 2.9.2, p 89, l 14f.
   It keeps OpenMP from automagically changing the number
   of threads used for parallel regions.
   */
  omp_set_dynamic( false );
#endif
  set_num_threads( 1 );
}

void
nest::VPManager::finalize()
{
}

void
nest::VPManager::set_status( const DictionaryDatum& d )
{
  long n_threads = get_num_threads();
  bool n_threads_updated =
    updateValue< long >( d, "local_num_threads", n_threads );
  if ( n_threads_updated )
  {
    if ( kernel().node_manager.size() > 1 )
      throw KernelException(
        "Nodes exist: Thread/process number cannot be changed." );
    if ( kernel().model_manager.has_user_models() )
      throw KernelException(
        "Custom neuron models exist: Thread/process number cannot be "
        "changed." );
    if ( kernel().model_manager.has_user_prototypes() )
      throw KernelException(
        "Custom synapse types exist: Thread/process number cannot be "
        "changed." );
    if ( kernel().connection_manager.get_user_set_delay_extrema() )
      throw KernelException(
        "Delay extrema have been set: Thread/process number cannot be "
        "changed." );
    if ( kernel().simulation_manager.has_been_simulated() )
      throw KernelException(
        "The network has been simulated: Thread/process number cannot be "
        "changed." );
    if ( not Time::resolution_is_default() )
      throw KernelException(
        "The resolution has been set: Thread/process number cannot be "
        "changed." );
    if ( kernel().model_manager.are_model_defaults_modified() )
      throw KernelException(
        "Model defaults have been modified: Thread/process number cannot be "
        "changed." );

    if ( n_threads > 1 && force_singlethreading_ )
    {
      LOG( M_WARNING,
        "VPManager::set_status",
        "No multithreading available, using single threading" );
      n_threads = 1;
    }

    // it is essential to call reset() here to adapt memory pools and more
    // to the new number of threads and VPs.
    set_num_threads( n_threads );
    kernel().num_threads_changed_reset();
  }

  long n_vps = get_num_virtual_processes();
  bool n_vps_updated =
    updateValue< long >( d, "total_num_virtual_procs", n_vps );
  if ( n_vps_updated )
  {
    if ( kernel().node_manager.size() > 1 )
      throw KernelException(
        "Nodes exist: Thread/process number cannot be changed." );
    if ( kernel().model_manager.has_user_models() )
      throw KernelException(
        "Custom neuron models exist: Thread/process number cannot be "
        "changed." );
    if ( kernel().model_manager.has_user_prototypes() )
      throw KernelException(
        "Custom synapse types exist: Thread/process number cannot be "
        "changed." );
    if ( kernel().connection_manager.get_user_set_delay_extrema() )
      throw KernelException(
        "Delay extrema have been set: Thread/process number cannot be "
        "changed." );
    if ( kernel().simulation_manager.has_been_simulated() )
      throw KernelException(
        "The network has been simulated: Thread/process number cannot be "
        "changed." );
    if ( not Time::resolution_is_default() )
      throw KernelException(
        "The resolution has been set: Thread/process number cannot be "
        "changed." );
    if ( kernel().model_manager.are_model_defaults_modified() )
      throw KernelException(
        "Model defaults have been modified: Thread/process number cannot be "
        "changed." );

    if ( n_vps % kernel().mpi_manager.get_num_processes() != 0 )
      throw BadProperty(
        "Number of virtual processes (threads*processes) must be an integer "
        "multiple of the number of processes. Value unchanged." );

    long n_threads = n_vps / kernel().mpi_manager.get_num_processes();
    if ( ( n_threads > 1 ) && ( force_singlethreading_ ) )
    {
      LOG( M_WARNING,
        "VPManager::set_status",
        "No multithreading available, using single threading" );
      n_threads = 1;
    }

    // it is essential to call reset() here to adapt memory pools and more
    // to the new number of threads and VPs
    set_num_threads( n_threads );
    kernel().num_threads_changed_reset();
  }
}

void
nest::VPManager::get_status( DictionaryDatum& d )
{
  def< long >( d, "local_num_threads", get_num_threads() );
  def< long >( d, "total_num_virtual_procs", get_num_virtual_processes() );
}

void
nest::VPManager::set_num_threads( nest::thread n_threads )
{
  n_threads_ = n_threads;

#ifdef _OPENMP
  omp_set_num_threads( n_threads_ );
#endif
}
