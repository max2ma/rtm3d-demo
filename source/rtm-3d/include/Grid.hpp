#ifndef GRID_H
#define GRID_H
#include <cstdlib>
#include <stdio.h>
#include <fstream>
#include <algorithm>
#include <vector>

using namespace std;

namespace grid
{

#define DEFAULT_PAGE_SIZE 4096
/**
 * @brief Class GridException Inheritance of class exception
 * @see RTMException.hpp
 */
class GridException : public exception
{
private:
    string what_msg; ///< Message for exception
public:
    /**
     * @brief Construct a new Grid Exception object
     * @param _what_msg 
     */
    GridException(string &_what_msg)
    {
        what_msg = _what_msg;
    }

    GridException(GridException &e)
    {
        what_msg = e.getWhatMsg();
    }
    ~GridException(){

    }
    GridException& operator=(GridException &e){
        what_msg = e.getWhatMsg();
        return *this;
    }
    string & getWhatMsg(){
        return what_msg;
    }

    /**
     * @brief Exception specifications
     * @return const char* string with explanatory information
     */    
    virtual const char *what() const throw()
    {
        return what_msg.c_str();
    }

    
};

/**
 * @brief Class Grid
 * @tparam GridData_type 
 */
template<typename GridData_type >
class Grid
{
protected:
    GridData_type * grid = nullptr;
public:
    size_t gridSize  = 0; /// total size
    GridData_type MAXVAL;
    GridData_type MINVAL;
    /**
     * @brief Construct a new Grid object
     */
    Grid(){
        grid = nullptr;
        gridSize = 0;
        MAXVAL = static_cast<GridData_type>(-10000000000.);
        MINVAL = static_cast<GridData_type>(10000000000.);
    }

    Grid(size_t _size)
    {
        gridSize = _size;
        grid = allocGrid(gridSize);
        //grid = new GridData_type[_size];
        MAXVAL = static_cast<GridData_type>(-10000000000.);
        MINVAL = static_cast<GridData_type>(10000000000.);
    }

    /**
     * @brief Construct a new Grid object
     * @param g
     */
    Grid(const Grid<GridData_type> &g)
    {
        gridSize = g.size();
        grid = allocGrid(gridSize);
        MAXVAL = g.MAXVAL;
        MINVAL = g.MINVAL;
        for (size_t i0 = 0; i0 < size; i0++)
        {
            (*this)[i0] = g[i0];
        }
    }
    /**
     * @brief Destroy the Grid object
     */
    ~Grid()
    {
        // if(grid!=nullptr){
        //     free(grid);
        //     grid = nullptr;
        // }
    }
    /**
     * @brief  Assignment operator
     * @param g  reference to Grid<GridData_type>
     * @return Grid& 
     */
    Grid &operator=(const Grid<GridData_type> &g)
    {
        gridSize = g.size();
        grid = allocGrid(gridSize);
        MAXVAL = g.MAXVAL;
        MINVAL = g.MINVAL;
        for (int i0 = 0; i0 < size; i0++)
        {
            (*this)[i0] = g[i0];
        }
        return *this;
    }
    GridData_type &operator[](int x) const
    {
        return Grid<GridData_type>::grid[x];
    }
    GridData_type *data() const
    {
        return Grid<GridData_type>::grid;
    }
    size_t size() const
    {
        return gridSize;
    }

    /**
     * @brief Get the By Offset object
     * @param offset 
     * @return T& 
     */
    GridData_type &getByOffset(size_t offset) const
    {
        return Grid<GridData_type>::grid[offset];
    }
    /**
     * @brief Set the By Offset object
     * @param offset 
     * @param val 
     */
    void setByOffset(size_t offset, GridData_type val)
    {
        Grid<GridData_type>::grid[offset] = val;
    }

    /**
     * @brief Function fill(): fills the entire grid data 
     *        with 'val'. Does not update device buffers.
     * @param val 
     */
    void fill(GridData_type val)
    {
        // std::fill(grid->begin(), grid->end(), val);
        size_t k;
        for (k=0; k<gridSize; k++)
            Grid<GridData_type>::grid[k] = val;
    }
    /**
     * @brief Set the Max Min object
     */
    void setMaxMin()
    {
        size_t i0 = 0;
        for (i0 = 0; i0 < gridSize; i0++)
        {
            if (Grid<GridData_type>::grid[i0] >= MAXVAL)
            {
                MAXVAL = Grid<GridData_type>::grid[i0];
            }
            if (Grid<GridData_type>::grid[i0] <= MINVAL)
            {
                MINVAL = Grid<GridData_type>::grid[i0];
            }
        }
    }
    /**
     * @brief Function multiply()
     * @param m
     */
    void multiplyBy(GridData_type m)
    {
        size_t i0 = 0;
        for (i0 = 0; i0 < gridSize; i0++)
        {
            Grid<GridData_type>::grid[i0] = Grid<GridData_type>::grid[i0] * m;
        }
    }

    void subtractBy(Grid<GridData_type> & _s){
        size_t i0 = 0;
        for (i0 = 0; i0 < gridSize; i0++)
        {
            Grid<GridData_type>::grid[i0] -= _s.getByOffset(i0);
        }
    }

    void power2(){
        size_t i0 = 0;
        for (i0 = 0; i0 < gridSize; i0++)
        {
            Grid<GridData_type>::grid[i0] = Grid<GridData_type>::grid[i0] * Grid<GridData_type>::grid[i0];
        }
    }

    /** aligned alloc **/
    GridData_type* allocGrid(const size_t _size) {
        void* ptr = nullptr;
        //if (posix_memalign(&ptr, DEFAULT_PAGE_SIZE, _size * sizeof(GridData_type))) throw std::bad_alloc();
#ifdef RTM_ACC_FPGA
        if(_size > DEFAULT_PAGE_SIZE){
            ptr = aligned_alloc(DEFAULT_PAGE_SIZE, _size * sizeof(GridData_type));
        }else{
            // ptr = malloc(_size * sizeof(GridData_type));
            ptr = static_cast<GridData_type *>(new GridData_type[_size]);
        }
#else
        ptr = static_cast<GridData_type *>(new GridData_type[_size]);
#endif
        if(ptr==nullptr)throw std::bad_alloc();
        return static_cast<GridData_type *>(ptr);
        
    }

    /**
     * @brief Function loadFromFile()
     * @param fname 
     */
    void loadFromFile(string &fname)
    {
        ifstream vfile(fname, ios::in | ios::binary);
        if (vfile.fail())
        {
            string msg = "Input grid file '" + fname + "' doesn't exist! Please check.";
            GridException ex(msg);
            throw ex; 
        }
        vfile.read(reinterpret_cast<char *>(this->data()), this->size() * sizeof(GridData_type));
        vfile.close();
    }
    /**
     * @brief Function loadFromFileAt()
     * @param fname 
     * @param cnt 
     * @param size 
     */
    void loadFromFileAt(string &fname, int cnt, size_t _size)
    {
        ifstream vfile(fname, ios::in | ios::binary);
        if (vfile.fail())
        {
            string msg = "Input grid file '" + fname + "' doesn't exist! Please check.";
            GridException ex(msg);
            throw ex; 
        }
        vfile.seekg(cnt * _size, ios::beg);
        vfile.read(reinterpret_cast<char *>(this->data()), this->size() * sizeof(GridData_type));
        vfile.close();
    }
    /**
     * @brief Function saveToFile()
     * @param fname 
     */
    void saveToFile(string &fname)
    {
        ofstream vfile(fname, ios::out | ios::binary);
        if (vfile.fail())
        {
            string msg = "Output grid file '" + fname + "' doesn't exist! Please check.";
            GridException ex(msg);
            throw ex; 
        }
        vfile.write(reinterpret_cast<char *>(this->data()), this->size() * sizeof(GridData_type));
        vfile.close();
    }
    /**
     * @brief Function appendTofile()
     * @param fname  
     */
    void appendTofile(string &fname)
    {
        ofstream vfile(fname, ios::out | ios::binary | ios::app);
        if (vfile.fail())
        {
            string msg = "Output grid file '" + fname + "' doesn't exist! Please check.";
            GridException ex(msg);
            throw ex; 
        }
        vfile.write(reinterpret_cast<char *>(grid), gridSize * sizeof(GridData_type));
        vfile.close();
    }
    /**
     * @brief Function initGrid()
     * @param dims 
     * @param len 
     */
    void initGrid(size_t _size){
        //delete grid;
        destroyGrid();
        gridSize = _size;
        grid = allocGrid(_size);
    }
    /**
     * @brief Function destroyGrid()
     */
    void destroyGrid(){
        if(grid!=nullptr){
#ifdef RTM_ACC_FPGA
            free(grid);
#else
            delete grid;
#endif
            grid = nullptr;
        }
        gridSize = 0;
    }

    /**
     * @brief Function destroyGrid()
     */
    void destroyGrid(GridData_type * l_grid){
        if(l_grid!=nullptr){
#ifdef RTM_ACC_FPGA
            free(l_grid);
#else
            delete l_grid;
#endif
            l_grid = nullptr;
        }
    }

    void copyData(Grid<GridData_type> &_from){
        memcpy(Grid<GridData_type>::data(), _from.data(), static_cast<size_t>(gridSize*sizeof(GridData_type)));
    }
};
} // namespace grid

#endif