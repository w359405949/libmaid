#pragma once

namespace maid {

class Context;
class ContextPool
{
public:


private:
    std::map< Context*, Context*> context_;

};

} // maid
