#include "mock.h"

maid::MockController* maid::MockController::default_instance_ = NULL;
maid::MockClosure* maid::MockClosure::default_instance_ = NULL;

namespace maid
{

void InitialMaidMock()
{
    maid::MockController::default_instance_ = new maid::MockController();
    maid::MockClosure::default_instance_ = new maid::MockClosure();
}

/*
 * force call InitialMaidMock
 */
struct StaticInitialMaidMock
{
    StaticInitialMaidMock()
    {
        InitialMaidMock();
    }
}static_initial_maid_mock_;

} /* namespace maid */
