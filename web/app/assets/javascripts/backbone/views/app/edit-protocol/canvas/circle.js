ChaiBioTech.app.Views = ChaiBioTech.app.Views || {}
ChaiBioTech.app.selectedCircle = null;

ChaiBioTech.app.Views.fabricCircle = function(model, parentStep) {

  this.model = model;
  this.parent = parentStep;
  this.canvas = parentStep.canvas;
  this.scrollTop = 80;
  this.scrollLength = 317;
  this.scrollRatio = (this.scrollLength - this.scrollTop) / 100;
  this.gatherDataImage = null;
  this.next = null;
  this.previous = null;
  this.big = false;
  this.controlDistance = ChaiBioTech.Constants.controlDistance;

  this.getLeft = function() {

    this.left = this.parent.left;
    return this;
  };

  this.getTop = function() {

    var temperature = this.temperatureValue || this.model.get("step").temperature;

    this.top = this.scrollLength - (temperature * this.scrollRatio);
    return this;
  };

  /*******************************************
    This method shows circles and gather data. Pease note
    this method is invoked from canvas.js once all the stage/step are loaded.
  ********************************************/
  this.getLinesAndCircles = function() {

    if(this.next) {
      this.curve = new ChaiBioTech.app.Views.fabricPath(model, this, this.canvas);
    }

    this.canvas.add(this.stepDataGroup);
    this.circleGroup.add(this.gatherDataImageMiddle);

    this.gatherDataOnScroll = new ChaiBioTech.app.Views.gatherDataGroupOnScroll(
      [
        this.gatherDataCircleOnScroll = new ChaiBioTech.app.Views.gatherDataCircleOnScroll(),
        this.gatherDataImageMiddle
      ], this);

    this.canvas.add(this.circleGroup);
    this.canvas.add(this.gatherDataOnScroll);

    this.gatherDataGroup = new ChaiBioTech.app.Views.gatherDataGroup(
      [
        this.gatherDataCircle = new ChaiBioTech.app.Views.gatherDataCircle(),
        this.gatherDataImage
      ], this);

    this.canvas.add(this.gatherDataGroup);
    this.showHideGatherData(this.parent.gatherDataDuringStep);
    this.gatherDataGroup.visible = this.parent.gatherDataDuringRamp;

    this.canvas.bringToFront(this.parent.rampSpeedGroup);
  };

  this.getUniqueId = function() {

    var name = this.parent.stepName.text;

    name = name + this.parent.parentStage.stageNo.text + "circle";
    this.uniqueName = name;
    return this;
  };

  this.doThingsForLast = function() {

    var holdTimeText = this.parent.holdDuration || this.model.get("step")["hold_time"];

    if(parseInt(holdTimeText) === 0) {
      this.holdTime.text = "∞";
    }
  };

  this.changeHoldTime = function() {

    var holdTimeHour = Math.floor(this.parent.holdDuration / 60);
    var holdTimeMinute = (this.parent.holdDuration % 60);

    if(holdTimeMinute < 10) {
      holdTimeMinute = "0" + holdTimeMinute;
    }

    if(holdTimeHour < 10) {
      holdTimeHour = "0" + holdTimeHour;
    }

    this.holdTime.text = holdTimeHour + ":" + holdTimeMinute;
  };

  this.render = function() {

    this.getLeft().getTop().getUniqueId();

    this.circleGroup = new ChaiBioTech.app.Views.circleGroup(
      [
        this.outerMostCircle = new ChaiBioTech.app.Views.outerMostCircle(),
        this.outerCircle = new ChaiBioTech.app.Views.outerCircle(),
        this.circle = new ChaiBioTech.app.Views.centerCircle(),
        this.littleCircleGroup = new ChaiBioTech.app.Views.littleCircleGroup(
          [
            this.littleCircle1 = new ChaiBioTech.app.Views.circleMaker(-10),
            this.littleCircle2 = new ChaiBioTech.app.Views.circleMaker(-2),
            this.littleCircle3 = new ChaiBioTech.app.Views.circleMaker(6)
          ]
        )
      ], this);

    this.stepDataGroup = new ChaiBioTech.app.Views.stepDataGroup([
        this.temperature = new ChaiBioTech.app.Views.stepTemperature(this.model, this),
        this.holdTime = new ChaiBioTech.app.Views.holdTime(this.model, this)
      ], this);
  };

  this.makeItBig = function() {

    this.big = true;

    if(this.parent.gatherDataDuringStep) {
      this.circle.setFill("#ffb400;");
      this.gatherDataImageMiddle.setVisible(false);
      this.gatherDataOnScroll.setVisible(true);
    }

    this.circle.setStroke("#ffb400");
    this.outerCircle.setStroke("black");
    this.outerCircle.strokeWidth = 7;
    this.stepDataGroup.setVisible(false);
    this.outerMostCircle.visible = this.littleCircleGroup.visible = true;
  };

  this.makeItSmall = function() {

    this.big = false;

    if(this.parent.gatherDataDuringStep) {
      this.circle.setFill("white");
      this.gatherDataImageMiddle.setVisible(true);
      this.gatherDataOnScroll.setVisible(false);
    }

    this.circle.setStroke("white");
    this.outerCircle.setStroke(null);
    this.stepDataGroup.setVisible(true);
    this.littleCircleGroup.visible = this.outerMostCircle.visible = false;
  };

  this.showHideGatherData = function(state) {

    if(state && ! this.big) {
        this.gatherDataImageMiddle.setVisible(state);
        this.circle.setFill("white");

    } else {
      this.circle.setFill("#ffb400");
      this.gatherDataOnScroll.setVisible(state);
    }
  };

  this.manageDrag = function(targetCircleGroup) {

    var top = targetCircleGroup.top;
    var left = targetCircleGroup.left;
    var previousTop = 0;

    if(top < this.scrollTop) {
      targetCircleGroup.setTop(this.scrollTop);
      this.manageRampLineMovement(left, this.scrollTop, targetCircleGroup);
    } else if(top > this.scrollLength) {
      targetCircleGroup.setTop(this.scrollLength);
      this.manageRampLineMovement(left, this.scrollLength, targetCircleGroup);
    } else {
      this.stepDataGroup.setTop(top + 55);
      this.gatherDataOnScroll.setTop(top - 26);
      this.manageRampLineMovement(left, top, targetCircleGroup);
    }
  };

  this.manageRampLineMovement = function(left, top, targetCircleGroup) {

    if(this.next) {
        this.curve.path[0][1] = left;
        this.curve.path[0][2] = top;
        // Calculating the mid point of the line at the right side of the circle
        // Remeber take the point which is static at the other side
        var endPointX = this.curve.path[2][3],
        endPointY = this.curve.path[2][4];

        var midPointX = (left + endPointX) / 2,
        midPointY = (top + endPointY) / 2;
        previousTop  = midPointY;

        this.curve.path[1][1] = left + this.controlDistance;
        this.curve.path[1][2] = top;

        // Mid point
        this.curve.path[1][3] = midPointX;
        this.curve.path[1][4] = midPointY;

        // We move the gather data Circle along with it [its next object's]
        this.next.gatherDataGroup.setTop(midPointY);

        // Controlling point for the next bent
        this.curve.path[2][1] = endPointX - this.controlDistance;
        this.curve.path[2][2] = endPointY;
    };

    if(this.previous) {
        previous = this.previous;
        previous.curve.path[2][3] = left;
        previous.curve.path[2][4] = top;
        // Calculating the mid point of the line at the left side of the cycle
        // Remeber take the point which is static at the other side
        var endPointX = previous.curve.path[0][1],
        endPointY = previous.curve.path[0][2];

        var midPointX = (left + endPointX) / 2,
        midPointY = (top + endPointY) / 2;

        previous.curve.path[2][1] = left - this.controlDistance;
        previous.curve.path[2][2] = top;

        // Mid point
        previous.curve.path[1][3] = midPointX;
        previous.curve.path[1][4] = midPointY;
        // We move the gather data Circle along with it
        // Please pay attention here we move gatherdta of this
        this.gatherDataGroup.setTop(midPointY);

        previous.curve.path[1][1] = endPointX + this.controlDistance;
        previous.curve.path[1][2] = endPointY;
    }
    // Change temperature display as its circle is moved
    var dynamicTemp = Math.abs((100 - ((targetCircleGroup.top - this.scrollTop) / this.scrollRatio)));

    dynamicTemp = (dynamicTemp < 100) ? dynamicTemp.toFixed(1) : dynamicTemp;
    this.temperature.text = String(dynamicTemp + "º");
  };

  this.changeRampSpeedPlacing = function(targetCircleGroup) {

    var parentStep = this.parent;
    var distance = Math.abs(targetCircleGroup.top - parentStep.rampSpeedGroup.top);

    if(distance < 50) {
      console.log("Closer", targetCircleGroup.top);
      if((parentStep.rampSpeedGroup.top + 70) > this.scrollLength) {
        parentStep.rampSpeedGroup.top = parentStep.rampSpeedGroup.top - 70;
      } else if((parentStep.rampSpeedGroup.top - 70) < this.scrollTop) {
        parentStep.rampSpeedGroup.top = parentStep.rampSpeedGroup.top + 70;
      } else {
        parentStep.rampSpeedGroup.top = parentStep.rampSpeedGroup.top + 70;
      }

    }
  };

  this.manageClick = function(starting) {

    this.makeItBig();
    this.parent.parentStage.selectStage();
    this.parent.selectStep();

    if(ChaiBioTech.app.selectedCircle) {
      var previousSelected = ChaiBioTech.app.selectedCircle;

      if(previousSelected.uniqueName != this.uniqueName) {
        previousSelected.makeItSmall();
      }
    }

    ChaiBioTech.app.selectedCircle = this;
    this.canvas.renderAll();
  };

  return this;
};