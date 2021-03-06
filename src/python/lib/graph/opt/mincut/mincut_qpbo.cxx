#include <pybind11/pybind11.h>

#include "nifty/graph/opt/mincut/mincut_objective.hxx"
#include "nifty/graph/opt/mincut/mincut_qpbo.hxx"

#include "nifty/python/graph/undirected_list_graph.hxx"
#include "nifty/python/graph/edge_contraction_graph.hxx"
#include "nifty/python/graph/opt/mincut/mincut_objective.hxx"
#include "nifty/python/converter.hxx"
#include "nifty/python/graph/opt/mincut/export_mincut_solver.hxx"

namespace py = pybind11;

PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);

namespace nifty{
namespace graph{
namespace opt{
namespace mincut{


    template<class OBJECTIVE>
    void exportMincutQpboT(py::module & module) {

        typedef OBJECTIVE ObjectiveType;
        typedef MincutQpbo<ObjectiveType> Solver;
        typedef typename Solver::SettingsType SettingsType;
        
        exportMincutSolver<Solver>(module,"MincutQpbo")
            .def(py::init<>())
            .def_readwrite("improve", &SettingsType::improve)
            //.def_readwrite("verbose", &SettingsType::verbose)
        ;
     
    }

    void exportMincutQpbo(py::module & module) {
        {
            typedef PyUndirectedGraph GraphType;
            typedef MincutObjective<GraphType, double> ObjectiveType;
            exportMincutQpboT<ObjectiveType>(module);
        }
        {
            typedef PyContractionGraph<PyUndirectedGraph> GraphType;
            typedef MincutObjective<GraphType, double> ObjectiveType;
            exportMincutQpboT<ObjectiveType>(module);
        }
    }

} // namespace nifty::graph::opt::mincut
} // namespace nifty::graph::opt
}
}
