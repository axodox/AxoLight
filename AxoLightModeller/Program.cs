using Microsoft.ML;
using Microsoft.ML.Data;
using Microsoft.VisualBasic.CompilerServices;
using System;
using System.IO;
using System.Linq;

namespace AxoLightModeller
{
  class Program
  {
    class RawColorInput
    {
      public byte R0 { get; set; }

      public byte G0 { get; set; }

      public byte B0 { get; set; }

      public byte R1 { get; set; }

      public byte G1 { get; set; }

      public byte B1 { get; set; }

      public float NR0 { get; set; }

      public float NG0 { get; set; }

      public float NB0 { get; set; }

      public float NR1 { get; set; }

      public float NG1 { get; set; }

      public float NB1 { get; set; }
    }

    class RawColorData
    {
      [LoadColumn(0)]
      public byte R0 { get; set; }

      [LoadColumn(1)]
      public byte G0 { get; set; }

      [LoadColumn(2)]
      public byte B0 { get; set; }

      [LoadColumn(3)]
      public byte R1 { get; set; }

      [LoadColumn(4)]
      public byte G1 { get; set; }

      [LoadColumn(5)]
      public byte B1 { get; set; }
    }

    class NormalizedColorData
    {
      public float NR0 { get; set; }

      public float NG0 { get; set; }

      public float NB0 { get; set; }

      public float NR1 { get; set; }

      public float NG1 { get; set; }

      public float NB1 { get; set; }
    }

    static void Main(string[] args)
    {
      var mlContext = new MLContext();
      var trainingData = mlContext.Data.LoadFromTextFile<RawColorData>(@"D:\Axodox\Documents\rgbMapping.csv", ',');


      var pipeline = mlContext.Transforms.Conversion.ConvertType(new[]
      {
        new InputOutputColumnPair("NR0", "R0"),
        new InputOutputColumnPair("NG0", "G0"),
        new InputOutputColumnPair("NB0", "B0"),
        new InputOutputColumnPair("NR1", "R1"),
        new InputOutputColumnPair("NG1", "G1"),
        new InputOutputColumnPair("NB1", "B1")
        }, DataKind.Single)
        .Append(mlContext.Transforms.Expression("NR0", "NR0 => NR0 / 255", "NR0"))
        .Append(mlContext.Transforms.Expression("NG0", "NG0 => NG0 / 255", "NG0"))
        .Append(mlContext.Transforms.Expression("NB0", "NB0 => NB0 / 255", "NB0"))
        .Append(mlContext.Transforms.Expression("NR1", "NR1 => NR1 / 255", "NR1"))
        .Append(mlContext.Transforms.Expression("NG1", "NG1 => NG1 / 255", "NG1"))
        .Append(mlContext.Transforms.Expression("NB1", "NB1 => NB1 / 255", "NB1"))
        .Append(mlContext.Transforms.CopyColumns("Label", "NB1"))
        .Append(mlContext.Transforms.Concatenate("Features", "NR0", "NG0", "NB0"))
        .Append(mlContext.Transforms.SelectColumns("Label", "Features"))
        .Append(mlContext.Regression.Trainers.LbfgsPoissonRegression());

      var model = pipeline.Fit(trainingData);
      var predictions = model.Transform(trainingData);

      var metrics = mlContext.Regression.Evaluate(predictions);

      var testValues = new RawColorData[100];
      for (var i = 0; i < testValues.Length; i++)
      {
        testValues[i] = new RawColorData()
        {
          B0 = (byte)(i / (float)testValues.Length * 255f)
        };
      }
      var testData = mlContext.Data.LoadFromEnumerable(testValues);
      var testOutput = model.Transform(testData);

      var valuesOut = testOutput.GetColumn<float>("Score").ToArray();

      var inputSchemaBuilder = new DataViewSchema.Builder();
      inputSchemaBuilder.AddColumn("R0", NumberDataViewType.Byte);
      inputSchemaBuilder.AddColumn("G0", NumberDataViewType.Byte);
      inputSchemaBuilder.AddColumn("B0", NumberDataViewType.Byte);
      var inputSchema = inputSchemaBuilder.ToSchema();
      mlContext.Model.Save(model, inputSchema, "test.bin");

      var exportData = mlContext.Data.LoadFromEnumerable(new[] { new RawColorInput() });

      using (var stream = new FileStream("test.onnx", FileMode.Create, FileAccess.Write))
      {
        OnnxExportExtensions.ConvertToOnnx(mlContext.Model, model, exportData, stream);
        stream.Flush();
      }
    }
  }
}
