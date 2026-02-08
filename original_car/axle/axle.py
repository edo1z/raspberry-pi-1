# Fusion 360 スクリプト - 前輪用軸（Axle）
# 両端ストッパー付き - 軸受けとホイールから抜けない設計

import adsk.core, adsk.fusion, adsk.cam, traceback, math

def run(context):
    ui = None
    try:
        app = adsk.core.Application.get()
        ui = app.userInterface
        design = app.activeProduct
        rootComp = design.rootComponent

        # === パラメータ設定（mm） ===
        axle_diameter = 5        # 軸の直径
        axle_length = 40         # 軸の長さ（ホイール25mm + 軸受け15mm）
        stopper_diameter = 8     # ストッパーの直径
        stopper_thickness = 2    # ストッパーの厚み

        # 単位をcmに変換
        axle_r = (axle_diameter / 2) / 10
        axle_len = axle_length / 10
        stopper_r = (stopper_diameter / 2) / 10
        stopper_t = stopper_thickness / 10

        # ========== コンポーネントを作成 ==========
        occ = rootComp.occurrences.addNewComponent(adsk.core.Matrix3D.create())
        axleComp = occ.component
        axleComp.name = "Axle_Front"

        sketches = axleComp.sketches
        xyPlane = axleComp.xYConstructionPlane
        extrudes = axleComp.features.extrudeFeatures
        centerPoint = adsk.core.Point3D.create(0, 0, 0)

        # ========== 軸本体を作成 ==========
        sketch1 = sketches.add(xyPlane)
        sketch1.sketchCurves.sketchCircles.addByCenterRadius(centerPoint, axle_r)

        axleProfile = sketch1.profiles.item(0)
        extInput = extrudes.createInput(axleProfile, adsk.fusion.FeatureOperations.NewBodyFeatureOperation)
        extInput.setDistanceExtent(False, adsk.core.ValueInput.createByReal(axle_len))
        extrudes.add(extInput)

        # ========== 内側ストッパー（原点側 = シャーシ/軸受け側）==========
        sketch2 = sketches.add(xyPlane)
        sketch2.sketchCurves.sketchCircles.addByCenterRadius(centerPoint, stopper_r)

        stopperProfile1 = sketch2.profiles.item(0)
        extInput2 = extrudes.createInput(stopperProfile1, adsk.fusion.FeatureOperations.JoinFeatureOperation)
        extInput2.setDistanceExtent(False, adsk.core.ValueInput.createByReal(stopper_t))
        extrudes.add(extInput2)

        # ========== 外側ストッパー（軸の反対端 = ホイール外側）==========
        # 軸の端にオフセット平面を作成
        planes = axleComp.constructionPlanes
        planeInput = planes.createInput()
        planeInput.setByOffset(xyPlane, adsk.core.ValueInput.createByReal(axle_len - stopper_t))
        endPlane = planes.add(planeInput)

        sketch3 = sketches.add(endPlane)
        sketch3.sketchCurves.sketchCircles.addByCenterRadius(centerPoint, stopper_r)

        stopperProfile2 = sketch3.profiles.item(0)
        extInput3 = extrudes.createInput(stopperProfile2, adsk.fusion.FeatureOperations.JoinFeatureOperation)
        extInput3.setDistanceExtent(False, adsk.core.ValueInput.createByReal(stopper_t))
        extrudes.add(extInput3)

        # ビューをフィット
        viewport = app.activeViewport
        viewport.fit()

        ui.messageBox(f'前輪用軸（両端ストッパー付き）完成！\n\n軸直径: {axle_diameter}mm\n軸長さ: {axle_length}mm\nストッパー直径: {stopper_diameter}mm\nストッパー厚み: {stopper_thickness}mm（両端）\n\n内側: 軸受けに引っかかる\n外側: ホイールに引っかかる\n\nコンポーネント名: Axle_Front')

    except:
        if ui:
            ui.messageBox('エラーが発生しました:\n{}'.format(traceback.format_exc()))
