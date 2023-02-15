//! \file
/*
**  This program is under the terms of the Apache License 2.0.
**  Jonathan Salwan
*/

#ifndef TRITON_ROUTINES_H
#define TRITON_ROUTINES_H


#include <triton/context.hpp>
#include "ttexplore.hpp"



//! The Triton namespace
namespace triton {
/*!
 *  \addtogroup triton
 *  @{
 */

  //! The Routines namespace
  namespace routines {
  /*!
   *  \ingroup triton
   *  \addtogroup routines
   *  @{
   */

    //! __libc_start_main routine
    triton::callbacks::cb_state_e __libc_start_main(triton::Context* ctx);
    //! printf routine
    triton::callbacks::cb_state_e printf(triton::Context* ctx);
    //! puts routine
    triton::callbacks::cb_state_e puts(triton::Context* ctx);
    //! fflush routine
    triton::callbacks::cb_state_e fflush(triton::Context* ctx);
    //! getlogin routine
    triton::callbacks::cb_state_e getlogin(triton::Context* ctx);
    //! usleep routine
    triton::callbacks::cb_state_e usleep(triton::Context* ctx);
    //! sleep routine
    triton::callbacks::cb_state_e sleep(triton::Context* ctx);
    //! putchar routine
    triton::callbacks::cb_state_e putchar(triton::Context* ctx);
    //! exit routine
    triton::callbacks::cb_state_e exit(triton::Context* ctx);
    //! strlen routine
    triton::callbacks::cb_state_e strlen(triton::Context* ctx);
    //! fgets routine
    triton::callbacks::cb_state_e fgets(triton::Context* ctx);
    //! stub routine
    triton::callbacks::cb_state_e stub(triton::Context* ctx);

  /*! @} End of routines namespace */
  };
/*! @} End of triton namespace */
};

#endif /* TRITON_ROUTINES_H */
