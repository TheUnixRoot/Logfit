#ifndef BARNESLOGFIT_IBODY_H
#define BARNESLOGFIT_IBODY_H

/*****************************************************************************
 * Interface IBody
 *
 * **************************************************************************/
class IBody {
public:
    virtual int GetVsize() = 0;
    // TODO: Fill this class with logic and methods
    virtual void OperatorCPU(int begin, int end) = 0;

    virtual void ShowCallback() = 0;

    virtual ~IBody() = default;

};

#endif //BARNESLOGFIT_IBODY_H
