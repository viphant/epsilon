#include <poincare/confidence_interval.h>
#include <poincare/addition.h>
#include <poincare/matrix.h>
#include <poincare/multiplication.h>
#include <poincare/power.h>
#include <poincare/layout_helper.h>
#include <poincare/serialization_helper.h>
#include <poincare/undefined.h>
#include <cmath>
#include <assert.h>

namespace Poincare {

constexpr Expression::FunctionHelper ConfidenceInterval::s_functionHelper;

constexpr Expression::FunctionHelper SimplePredictionInterval::s_functionHelper;

int ConfidenceIntervalNode::numberOfChildren() const { return ConfidenceInterval::s_functionHelper.numberOfChildren(); }

Layout ConfidenceIntervalNode::createLayout(Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const {
  return LayoutHelper::Prefix(ConfidenceInterval(this), floatDisplayMode, numberOfSignificantDigits, ConfidenceInterval::s_functionHelper.name());
}

int ConfidenceIntervalNode::serialize(char * buffer, int bufferSize, Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const {
  return SerializationHelper::Prefix(this, buffer, bufferSize, floatDisplayMode, numberOfSignificantDigits, ConfidenceInterval::s_functionHelper.name());
}

Expression ConfidenceIntervalNode::shallowReduce(ReductionContext reductionContext) {
  return ConfidenceInterval(this).shallowReduce(reductionContext);
}

template<typename T>
Evaluation<T> ConfidenceIntervalNode::templatedApproximate(Context * context, Preferences::ComplexFormat complexFormat, Preferences::AngleUnit angleUnit) const {
  Evaluation<T> fInput = childAtIndex(0)->approximate(T(), context, complexFormat, angleUnit);
  Evaluation<T> nInput = childAtIndex(1)->approximate(T(), context, complexFormat, angleUnit);
  T f = static_cast<Complex<T> &>(fInput).toScalar();
  T n = static_cast<Complex<T> &>(nInput).toScalar();
  if (std::isnan(f) || std::isnan(n) || n != (int)n || n < 0 || f < 0 || f > 1) {
    return Complex<T>::Undefined();
  }
  std::complex<T> operands[2];
  operands[0] = std::complex<T>(f - 1/std::sqrt(n));
  operands[1] = std::complex<T>(f + 1/std::sqrt(n));
  return MatrixComplex<T>::Builder(operands, 1, 2);
}

Layout SimplePredictionIntervalNode::createLayout(Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const {
  return LayoutHelper::Prefix(SimplePredictionInterval(this), floatDisplayMode, numberOfSignificantDigits, SimplePredictionInterval::s_functionHelper.name());
}

int SimplePredictionIntervalNode::serialize(char * buffer, int bufferSize, Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const {
  return SerializationHelper::Prefix(this, buffer, bufferSize, floatDisplayMode, numberOfSignificantDigits, SimplePredictionInterval::s_functionHelper.name());
}

Expression ConfidenceInterval::shallowReduce(ExpressionNode::ReductionContext reductionContext) {
  {
    Expression e = Expression::defaultShallowReduce();
    if (e.isUndefined()) {
      return e;
    }
  }
  Expression c0 = childAtIndex(0);
  Expression c1 = childAtIndex(1);
  if (SortedIsMatrix(c0, reductionContext.context()) || SortedIsMatrix(c1, reductionContext.context())) {
    Expression result = Undefined::Builder();
    replaceWithInPlace(result);
    return result;
  }
  if (c0.type() == ExpressionNode::Type::Rational) {
    Rational r0 = static_cast<Rational&>(c0);
    if (r0.signedIntegerNumerator().isNegative() || Integer::NaturalOrder(r0.signedIntegerNumerator(), r0.integerDenominator()) > 0) {
      Expression result = Undefined::Builder();
      replaceWithInPlace(result);
      return result;
    }
  }
  if (c1.type() == ExpressionNode::Type::Rational) {
    Rational r1 = static_cast<Rational&>(c1);
    if (!r1.integerDenominator().isOne() || r1.signedIntegerNumerator().isNegative()) {
      Expression result = Undefined::Builder();
      replaceWithInPlace(result);
      return result;
    }
  }
  if (c0.type() != ExpressionNode::Type::Rational || c1.type() != ExpressionNode::Type::Rational) {
    return *this;
  }
  Rational r0 = static_cast<Rational&>(c0);
  Rational r1 = static_cast<Rational&>(c1);
  // Compute [r0-1/sqr(r1), r0+1/sqr(r1)]
  Expression sqr = Power::Builder(r1, Rational::Builder(-1, 2));
  Matrix matrix = Matrix::Builder();
  matrix.addChildAtIndexInPlace(Addition::Builder(r0.clone(), Multiplication::Builder(Rational::Builder(-1), sqr.clone())), 0, 0);
  matrix.addChildAtIndexInPlace(Addition::Builder(r0, sqr), 1, 1);
  matrix.setDimensions(1, 2);
  replaceWithInPlace(matrix);
  matrix.deepReduceChildren(reductionContext);
  return matrix;
}


}
