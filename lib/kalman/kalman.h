/*
 * kalman.h
 *
 *  Created on: Dec 30, 2020
 *      Author: Vamshi
 */

#ifndef KALMAN_H_
#define KALMAN_H_

//#include <Arduino.h>
#include <Dense>
#include <Core>

using namespace std;
using namespace Eigen;

class Kalman {
    public:
      Kalman(int stateDim, MatrixXd F, int measurementDim, int inputDim,
	    MatrixXd H, MatrixXd B, MatrixXd Q, MatrixXd R, VectorXd initX);
      ~Kalman();
      void predict(VectorXd u);
      void update(VectorXd z);
      VectorXd _x;
      MatrixXd __P; // nxn
      MatrixXd _K; // nxm
    private:
      int _n;
      int _m;
      int _l; // size of input vector
      MatrixXd _F; // nxn
      MatrixXd _H; // mxn
      MatrixXd __B; // nxl
      MatrixXd _Q; // nxn
      MatrixXd _R; // mxm
      MatrixXd __S; // mxm

      VectorXd _y;
};

#endif /* KALMAN_H_ */
