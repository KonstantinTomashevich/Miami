#pragma once

#include <unordered_map>

#include <Miami/Annotations.hpp>

#include <Miami/Disco/Disco.hpp>

#include <Miami/Richard/Data.hpp>
#include <Miami/Richard/Table.hpp>

namespace Miami::Richard
{
// TODO: Add tests for whole Richard library. Don't have time now because of tight deadline.
class Conduit final
{
public:
    explicit Conduit (Disco::Context *multithreadingContext);

    free_call Disco::ReadWriteGuard &ReadWriteGuard ();

    free_call ResultCode GetTableIds (const std::shared_ptr <Disco::SafeLockGuard> &readOrWriteGuard,
                                      std::vector <AnyDataId> &output) const;

    free_call ResultCode GetTable (const std::shared_ptr <Disco::SafeLockGuard> &readOrWriteGuard,
                                   AnyDataId id, Table *&output);

    free_call ResultCode AddTable (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard,
                                   const std::string &name, AnyDataId &outputId);

    free_call ResultCode RemoveTable (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard,
                                      const std::shared_ptr <Disco::SafeLockGuard> &tableWriteGuard,
                                      AnyDataId tableId);

private:
    free_call bool CheckReadOrWriteGuard (const std::shared_ptr <Disco::SafeLockGuard> &readOrWriteGuard) const;

    free_call bool CheckWriteGuard (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard) const;

    Disco::ReadWriteGuard guard_;

    // TODO: Unique pointer because moving structures with Disco locks is unsafe for now.
    std::unordered_map <AnyDataId, std::unique_ptr <Table>> tables_;
    AnyDataId nextTableId_;
};
}