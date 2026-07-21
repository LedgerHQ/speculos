from ..mcu.struct import MODELS
from .restful import ScreenResource


class ModelInfo(ScreenResource):
    def get(self):
        model = self.screen.display.model
        width, height = MODELS[model].screen_size
        return {"name": model, "screen": {"width": width, "height": height}}, 200
