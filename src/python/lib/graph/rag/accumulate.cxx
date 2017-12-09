#include <cstddef>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

#ifdef WITH_HDF5
#include "nifty/hdf5/hdf5_array.hxx"
#endif

#include "nifty/graph/rag/grid_rag.hxx"
#include "nifty/graph/rag/grid_rag_labels_proxy.hxx"
#include "nifty/graph/rag/grid_rag_accumulate.hxx"

#include "xtensor-python/pytensor.hpp"
#include "nifty/xtensor/xtensor.hxx"

// still need this for python bindings of nifty::ArrayExtender
#include "nifty/python/converter.hxx"

namespace py = pybind11;


namespace nifty{
namespace graph{

    using namespace py;

    template<std::size_t DIM, class RAG, class CONTR_GRAP, class DATA_T>
    void exportAccumulateAffinitiesMeanAndLength(
        py::module & ragModule
    ){
        ragModule.def("accumulateAffinities",
        [](
            const RAG & rag,
            xt::pytensor<DATA_T, DIM+1> affinities,
            xt::pytensor<int, 2> offsets
        ){
            const auto & labels = rag.labelsProxy().labels();
            const auto & shape = rag.labelsProxy().shape();

            xt::pytensor<DATA_T, 1> accAff({uint64_t(rag.edgeIdUpperBound()+1)});
            xt::pytensor<DATA_T, 1> counter({uint64_t(rag.edgeIdUpperBound()+1)});

            auto & offsetShape = offsets.shape();

            std::fill(accAff.begin(), accAff.end(), 0);
            std::fill(counter.begin(), counter.end(), 0);

            for(auto x=0; x<shape[0]; ++x){
                for(auto y=0; y<shape[1]; ++y){
                    if (DIM==3){
                        for(auto z=0; z<shape[2]; ++z){

                            const auto u = labels(x,y,z);

                            for(auto i = 0; i < offsetShape[0]; ++i){
                                const auto ox = offsets(i, 0);
                                const auto oy = offsets(i, 1);
                                const auto oz = offsets(i, 2);
                                const auto xx = ox +x ;
                                const auto yy = oy +y ;
                                const auto zz = oz +z ;


                                if(xx>=0 && xx<shape[0] && yy >=0 && yy<shape[1] && zz >=0 && zz<shape[2]){
                                    const auto v = labels(xx,yy,zz);
                                    if(u != v){
                                        const auto edge = rag.findEdge(u,v);
                                        if(edge >=0 ){
                                            counter[edge] += 1.;
                                            // accAff[edge] = 0.;
                                            accAff[edge] += affinities(x,y,z,i);
                                        }
                                    }
                                }
                            }
                        }
                    } else if(DIM==2) {
                        const auto u = labels(x,y);

                        for(auto i=0; i<offsetShape[0]; ++i){
                            const auto ox = offsets(i, 0);
                            const auto oy = offsets(i, 1);

                            const auto xx = ox +x;
                            const auto yy = oy +y;

                            if(xx>=0 && xx<shape[0] && yy >=0 && yy<shape[1]){
                                const auto v = labels(xx,yy);
                                if(u != v){
                                    const auto edge = rag.findEdge(u,v);
                                    if(edge >=0 ){
                                        counter[edge] +=1.;
                                        // accAff[edge] = 0.;
                                        accAff[edge] += affinities(x,y,i);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Normalize:
            for(auto i=0; i<uint64_t(rag.edgeIdUpperBound()+1); ++i){
                if(counter[i]!=0){
                    accAff[i] /= counter[i];
                }
            }
            return accAff;

        },
        py::arg("rag"),
        py::arg("affinities"),
        py::arg("offsets")
        );

    }


    template<std::size_t DIM, class RAG, class DATA_T>
    void exportAccumulateEdgeMeanAndLength(
        py::module & ragModule
    ){
        ragModule.def("accumulateEdgeMeanAndLength",
        [](
            const RAG & rag,
            xt::pytensor<DATA_T, DIM> data,
            array::StaticArray<int64_t, DIM> blockShape,
            const int numberOfThreads
        ){

            xt::pytensor<DATA_T, 2> out({int64_t(rag.edgeIdUpperBound()+1), int64_t(2)});
            {
                py::gil_scoped_release allowThreads;
                array::StaticArray<int64_t, DIM> blockShape_;
                accumulateEdgeMeanAndLength(rag, data, blockShape, out, numberOfThreads);
            }
            return out;
        },
        py::arg("rag"),
        py::arg("data"),
        py::arg("blockShape") = array::StaticArray<int64_t,DIM>(100),
        py::arg("numberOfThreads")= -1
        );
    }


    template<std::size_t DIM, class RAG, class DATA_T>
    void exportAccumulateGeometricEdgeFeatures(
        py::module & ragModule
    ){
        ragModule.def("accumulateGeometricEdgeFeatures",
        [](
            const RAG & rag,
            array::StaticArray<int64_t, DIM> blockShape,
            const int numberOfThreads
        ){

            xt::pytensor<DATA_T, 2> out({int64_t(rag.edgeIdUpperBound()+1), int64_t(17)});
            {
                py::gil_scoped_release allowThreads;
                array::StaticArray<int64_t, DIM> blockShape_;
                accumulateGeometricEdgeFeatures(rag, blockShape, out, numberOfThreads);
            }
            return out;
        },
        py::arg("rag"),
        py::arg("blockShape") = array::StaticArray<int64_t,DIM>(100),
        py::arg("numberOfThreads")= -1
        );
    }


    template<std::size_t DIM, class RAG, class DATA_T>
    void exportAccumulateMeanAndLength(
        py::module & ragModule
    ){
        ragModule.def("accumulateMeanAndLength",
        [](
            const RAG & rag,
            xt::pytensor<DATA_T, DIM> data,
            array::StaticArray<int64_t, DIM> blockShape,
            const int numberOfThreads,
            const bool saveMemory
        ){
            typedef xt::pytensor<DATA_T, 2> NumpyArrayType;
            typedef std::pair<NumpyArrayType, NumpyArrayType>  OutType;
            NumpyArrayType edgeOut({int64_t(rag.edgeIdUpperBound()+1), int64_t(2)});
            NumpyArrayType nodeOut({int64_t(rag.nodeIdUpperBound()+1), int64_t(2)});
            {
                py::gil_scoped_release allowThreads;
                array::StaticArray<int64_t, DIM> blockShape_;
                accumulateMeanAndLength(rag, data, blockShape, edgeOut, nodeOut, numberOfThreads);
            }
            return OutType(edgeOut, nodeOut);;
        },
        py::arg("rag"),
        py::arg("data"),
        py::arg("blockShape") = array::StaticArray<int64_t,DIM>(100),
        py::arg("numberOfThreads")= -1,
        py::arg_t<bool>("saveMemory",false)
        );
    }

    #ifdef WITH_HDF5
    template<std::size_t DIM, class RAG, class DATA_T>
    void exportAccumulateMeanAndLengthHdf5(
        py::module & ragModule
    ){
        ragModule.def("accumulateMeanAndLength",
        [](
            const RAG & rag,
            const nifty::hdf5::Hdf5Array<DATA_T> & data,
            array::StaticArray<int64_t, DIM> blockShape,
            const int numberOfThreads,
            const bool saveMemory
        ){
            typedef xt::pytensor<DATA_T, 2> NumpyArrayType;
            typedef std::pair<NumpyArrayType, NumpyArrayType>  OutType;
            NumpyArrayType edgeOut({int64_t(rag.edgeIdUpperBound()+1), int64_t(2)});
            NumpyArrayType nodeOut({int64_t(rag.nodeIdUpperBound()+1), int64_t(2)});
            {
                py::gil_scoped_release allowThreads;
                array::StaticArray<int64_t, DIM> blockShape_;
                accumulateMeanAndLength(rag, data, blockShape, edgeOut, nodeOut, numberOfThreads);
            }
            return OutType(edgeOut, nodeOut);;
        },
        py::arg("rag"),
        py::arg("data"),
        py::arg("blockShape") = array::StaticArray<int64_t,DIM>(100),
        py::arg("numberOfThreads")= -1,
        py::arg_t<bool>("saveMemory",false)
        );
    }
    #endif




    template<std::size_t DIM, class RAG, class DATA_T>
    void exportAccumulateStandartFeatures(
        py::module & ragModule
    ){
        ragModule.def("accumulateStandartFeatures",
        [](
            const RAG & rag,
            xt::pytensor<DATA_T, DIM> data,
            const double minVal,
            const double maxVal,
            array::StaticArray<int64_t, DIM> blockShape,
            const int numberOfThreads
        ){
            typedef xt::pytensor<DATA_T, 2> NumpyArrayType;
            typedef std::pair<NumpyArrayType, NumpyArrayType>  OutType;
            NumpyArrayType edgeOut({int64_t(rag.edgeIdUpperBound()+1), int64_t(9)});
            NumpyArrayType nodeOut({int64_t(rag.nodeIdUpperBound()+1), int64_t(9)});
            {
                py::gil_scoped_release allowThreads;
                array::StaticArray<int64_t, DIM> blockShape_;
                accumulateStandartFeatures(rag, data, minVal, maxVal, blockShape, edgeOut, nodeOut, numberOfThreads);
            }
            return OutType(edgeOut, nodeOut);
        },
        py::arg("rag"),
        py::arg("data"),
        py::arg("minVal"),
        py::arg("maxVal"),
        py::arg("blockShape") = array::StaticArray<int64_t,DIM>(100),
        py::arg("numberOfThreads")= -1
        );
    }

    #ifdef WITH_HDF5
    template<std::size_t DIM, class RAG, class DATA_T>
    void exportAccumulateStandartFeaturesHdf5(
        py::module & ragModule
    ){
        ragModule.def("accumulateStandartFeatures",
        [](
            const RAG & rag,
            const nifty::hdf5::Hdf5Array<DATA_T> & data,
            const double minVal,
            const double maxVal,
            array::StaticArray<int64_t, DIM> blockShape,
            const int numberOfThreads
        ){
            typedef xt::pytensor<DATA_T, 2> NumpyArrayType;
            typedef std::pair<NumpyArrayType, NumpyArrayType>  OutType;
            NumpyArrayType edgeOut({int64_t(rag.edgeIdUpperBound()+1), int64_t(9)});
            NumpyArrayType nodeOut({int64_t(rag.nodeIdUpperBound()+1), int64_t(9)});
            {
                py::gil_scoped_release allowThreads;
                array::StaticArray<int64_t, DIM> blockShape_;
                accumulateStandartFeatures(rag, data, minVal, maxVal, blockShape, edgeOut, nodeOut, numberOfThreads);
            }
            return OutType(edgeOut, nodeOut);
        },
        py::arg("rag"),
        py::arg("data"),
        py::arg("minVal"),
        py::arg("maxVal"),
        py::arg("blockShape") = array::StaticArray<int64_t,DIM>(100),
        py::arg("numberOfThreads")= -1
        );
    }

    #endif




    template<std::size_t DIM, class RAG, class DATA_T>
    void exportAccumulateNodeStandartFeatures(
        py::module & ragModule
    ){
        ragModule.def("accumulateNodeStandartFeatures",
        [](
            const RAG & rag,
            xt::pytensor<DATA_T, DIM> data,
            const double minVal,
            const double maxVal,
            array::StaticArray<int64_t, DIM> blockShape,
            const int numberOfThreads
        ){
            typedef xt::pytensor<DATA_T, 2> NumpyArrayType;
            NumpyArrayType nodeOut({int64_t(rag.nodeIdUpperBound()+1), int64_t(9)});
            {
                py::gil_scoped_release allowThreads;
                accumulateNodeStandartFeatures(rag, data, minVal, maxVal, blockShape, nodeOut, numberOfThreads);
            }
            return nodeOut;
        },
        py::arg("rag"),
        py::arg("data"),
        py::arg("minVal"),
        py::arg("maxVal"),
        py::arg("blockShape") = array::StaticArray<int64_t,DIM>(100),
        py::arg("numberOfThreads")= -1
        );
    }

    template<std::size_t DIM, class RAG, class DATA_T>
    void exportAccumulateEdgeStandartFeatures(
        py::module & ragModule
    ){
        ragModule.def("accumulateEdgeStandartFeatures",
        [](
            const RAG & rag,
            xt::pytensor<DATA_T, DIM> data,
            const double minVal,
            const double maxVal,
            array::StaticArray<int64_t, DIM> blockShape,
            const int numberOfThreads
        ){
            typedef xt::pytensor<DATA_T, 2> NumpyArrayType;
            NumpyArrayType edgeOut({int64_t(rag.edgeIdUpperBound()+1), int64_t(9)});
            {
                py::gil_scoped_release allowThreads;
                accumulateEdgeStandartFeatures(rag, data, minVal, maxVal, blockShape, edgeOut, numberOfThreads);
            }
            return edgeOut;
        },
        py::arg("rag"),
        py::arg("data"),
        py::arg("minVal"),
        py::arg("maxVal"),
        py::arg("blockShape") = array::StaticArray<int64_t,DIM>(100),
        py::arg("numberOfThreads")= -1
        );
    }



    template<std::size_t DIM, class RAG, class DATA_T>
    void exportAccumulateGeometricNodeFeatures(
        py::module & ragModule
    ){
        ragModule.def("accumulateGeometricNodeFeatures",
        [](
            const RAG & rag,
            array::StaticArray<int64_t, DIM> blockShape,
            const int numberOfThreads
        ){
            typedef xt::pytensor<DATA_T, 2> NumpyArrayType;
            NumpyArrayType nodeOut({int64_t(rag.nodeIdUpperBound()+1), int64_t(3*DIM+1)});
            {
                py::gil_scoped_release allowThreads;
                accumulateGeometricNodeFeatures(rag, blockShape, nodeOut, numberOfThreads);
            }
            return nodeOut;
        },
        py::arg("rag"),
        py::arg("blockShape") = array::StaticArray<int64_t,DIM>(100),
        py::arg("numberOfThreads")= -1
        );
    }



    void exportAccumulate(py::module & ragModule) {

        //explicit
        {
            typedef LabelsProxy<2, xt::pytensor<uint32_t, 2>> ExplicitPyLabels2D;
            typedef GridRag<2, ExplicitPyLabels2D> Rag2d;

            typedef LabelsProxy<3, xt::pytensor<uint32_t, 3>> ExplicitPyLabels3D;
            typedef GridRag<3, ExplicitPyLabels3D> Rag3d;

            exportAccumulateEdgeMeanAndLength<2, Rag2d, float>(ragModule);
            exportAccumulateEdgeMeanAndLength<3, Rag3d, float>(ragModule);

            exportAccumulateMeanAndLength<2, Rag2d, float>(ragModule);
            exportAccumulateMeanAndLength<3, Rag3d, float>(ragModule);

            exportAccumulateStandartFeatures<2, Rag2d, float>(ragModule);
            exportAccumulateStandartFeatures<3, Rag3d, float>(ragModule);

            exportAccumulateNodeStandartFeatures<2, Rag2d, float>(ragModule);
            exportAccumulateNodeStandartFeatures<3, Rag3d, float>(ragModule);

            exportAccumulateEdgeStandartFeatures<2, Rag2d, float>(ragModule);
            exportAccumulateEdgeStandartFeatures<3, Rag3d, float>(ragModule);

            exportAccumulateGeometricNodeFeatures<2, Rag2d, float>(ragModule);
            exportAccumulateGeometricNodeFeatures<3, Rag3d, float>(ragModule);

            exportAccumulateGeometricEdgeFeatures<2, Rag2d, float>(ragModule);
            exportAccumulateGeometricEdgeFeatures<3, Rag3d, float>(ragModule);

            #ifdef WITH_HDF5
            typedef GridRag<3, Hdf5Labels<3, uint32_t>> RagH53d;
            //exportAccumulateMeanAndLengthHdf5<3,RagH53d, float>(ragModule);
            exportAccumulateStandartFeaturesHdf5<3, RagH53d, uint8_t>(ragModule);
            #endif

        }
    }

} // end namespace graph
} // end namespace nifty
